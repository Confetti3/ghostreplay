#include "capture/CaptureManager.h"
#include "capture/CaptureTarget.h"
#include "capture/DXGICapture.h"
#include "capture/WGCCapture.h"
#include "encode/EncoderSelector.h"
#include "encode/FFmpegVideoEncoder.h"
#include "buffer/RingBuffer.h"
#include "game/GameDetector.h"
#include "util/Logger.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <wrl/client.h>

#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

CaptureManager::CaptureManager() = default;
CaptureManager::~CaptureManager()
{
    stop();
}

// ── Select adapter for the given monitor ──────────────────────

static HMONITOR getMonitorByIndex(int index)
{
    struct Ctx { int target; int current; HMONITOR result; };
    Ctx ctx = { index, 0, nullptr };

    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR hmon, HDC, LPRECT, LPARAM lp) -> BOOL {
            auto* c = reinterpret_cast<Ctx*>(lp);
            if (c->current == c->target) { c->result = hmon; return FALSE; }
            c->current++;
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));

    return ctx.result;
}

static ComPtr<IDXGIAdapter1> getAdapterForMonitor(IDXGIFactory1* factory, HMONITOR hmon)
{
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hmon, &mi)) return nullptr;

    for (UINT i = 0; ; i++)
    {
        ComPtr<IDXGIAdapter1> adapter;
        if (FAILED(factory->EnumAdapters1(i, &adapter))) break;

        for (UINT j = 0; ; j++)
        {
            ComPtr<IDXGIOutput> output;
            if (FAILED(adapter->EnumOutputs(j, &output))) break;

            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(output->GetDesc(&desc)) &&
                wcscmp(desc.DeviceName, mi.szDevice) == 0)
            {
                return adapter;
            }
        }
    }
    return nullptr;
}

static GpuVendor vendorFromDxgiId(UINT vendor_id)
{
    switch (vendor_id)
    {
    case 0x10DE: return GpuVendor::Nvidia;
    case 0x1002: return GpuVendor::AMD;
    case 0x8086: return GpuVendor::Intel;
    default: return GpuVendor::Unknown;
    }
}

static std::string sourceDisplayName(CaptureSource source)
{
    switch (source)
    {
    case CaptureSource::Window: return "Window";
    case CaptureSource::Game: return "Game";
    case CaptureSource::Desktop:
    default: return "Desktop";
    }
}

static std::string wideToUtf8(const wchar_t* value)
{
    if (!value || !*value)
        return {};

    const int required = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0)
        return {};

    std::string result(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), required, nullptr, nullptr);
    return result;
}

static int encoderSafeDimension(int value)
{
    if (value <= 0)
        return value;
    if (value < 2)
        return 2;
    return value & ~1;
}

// ── Initialize D3D11 ──────────────────────────────────────────

bool CaptureManager::initialize(int monitor_index, const EncoderConfig& enc_config,
                                const std::string& capture_source,
                                void* owner_window,
                                bool allow_window_picker)
{
    CaptureSource source = captureSourceFromString(capture_source);
    HWND target_window = nullptr;
    m_size_mismatch_warned = false;

    if (source == CaptureSource::Window && !allow_window_picker)
    {
        LOG_WARNING("CaptureManager: window capture requires an interactive picker; refusing non-interactive startup");
        return false;
    }

    if (source == CaptureSource::Game)
    {
        target_window = GameDetector::foregroundWindow();
        if (!GameDetector::isWindowCapturable(target_window))
        {
            LOG_WARNING("CaptureManager: no capturable foreground game window; falling back to desktop capture");
            source = CaptureSource::Desktop;
            target_window = nullptr;
        }
    }

    // Get target monitor
    HMONITOR hmon = getMonitorByIndex(monitor_index);
    if (target_window)
        hmon = MonitorFromWindow(target_window, MONITOR_DEFAULTTONEAREST);
    if (!hmon)
    {
        LOG_ERROR("CaptureManager: monitor " + std::to_string(monitor_index) + " not found");
        return false;
    }

    EncoderConfig actual_enc_config = enc_config;
    const bool has_width_override = actual_enc_config.width > 0;
    const bool has_height_override = actual_enc_config.height > 0;
    int monitor_width = 0;
    int monitor_height = 0;

    MONITORINFOEXW monitor_info = {};
    monitor_info.cbSize = sizeof(monitor_info);
    if (GetMonitorInfoW(hmon, &monitor_info))
    {
        monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
        monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
    }

    // Create DXGI factory
    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr))
    {
        LOG_ERROR("CaptureManager: CreateDXGIFactory1 failed, hr=0x" +
                  std::to_string(hr));
        return false;
    }
    m_dxgi_factory = factory;

    // Find the adapter driving the monitor
    ComPtr<IDXGIAdapter1> adapter = getAdapterForMonitor(factory.Get(), hmon);
    if (!adapter)
    {
        LOG_WARNING("CaptureManager: could not match adapter to monitor, using default");
    }

    GpuVendor gpu_vendor = GpuVendor::Unknown;

    // Adapter description for logging
    if (adapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        gpu_vendor = vendorFromDxgiId(desc.VendorId);
        LOG_INFO("CaptureManager: adapter = " + wideToUtf8(desc.Description));
    }

    // Create D3D11 device with required flags
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL selected_level;

    hr = D3D11CreateDevice(
        adapter.Get(),
        adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        create_flags,
        feature_levels,
        ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION,
        &device,
        &selected_level,
        &context);

    if (FAILED(hr))
    {
        LOG_ERROR("CaptureManager: D3D11CreateDevice failed, hr=0x" +
                  std::to_string(hr));
        return false;
    }

    m_device = device;
    m_context = context;

    LOG_INFO("CaptureManager: D3D11 device created, feature level " +
             std::to_string(selected_level >> 12) + "_" +
             std::to_string((selected_level >> 8) & 0xF));

    // ── Create capture backend ───────────────────────────────

    m_capture_source_name = captureSourceToString(source);
    m_capture_target_name = sourceDisplayName(source);

    if (source == CaptureSource::Desktop)
    {
        auto dxgi_capture = std::make_unique<DXGICapture>();
        if (dxgi_capture->initialize(m_device.Get(), hmon))
        {
            m_capture = std::move(dxgi_capture);
            m_capture_target_name = "Desktop";
        }
        else
        {
            LOG_WARNING("CaptureManager: DXGI desktop capture failed, trying Windows Graphics Capture");
            auto wgc_capture = std::make_unique<WGCCapture>();
            if (!wgc_capture->initializeForMonitor(m_device.Get(), hmon))
            {
                LOG_ERROR("CaptureManager: desktop capture initialization failed");
                return false;
            }
            m_capture_target_name = wgc_capture->targetName();
            m_capture = std::move(wgc_capture);
        }
    }
    else if (source == CaptureSource::Window)
    {
        auto wgc_capture = std::make_unique<WGCCapture>();
        const HWND owner = static_cast<HWND>(owner_window);
        if (!wgc_capture->initializeWithPicker(m_device.Get(), owner))
        {
            LOG_ERROR("CaptureManager: window capture picker was cancelled or failed");
            return false;
        }
        m_capture_target_name = wgc_capture->targetName();
        m_capture = std::move(wgc_capture);
    }
    else
    {
        auto wgc_capture = std::make_unique<WGCCapture>();
        if (!wgc_capture->initializeForWindow(m_device.Get(), target_window))
        {
            LOG_WARNING("CaptureManager: game window capture failed, falling back to desktop capture");
            source = CaptureSource::Desktop;
            m_capture_source_name = captureSourceToString(source);
            auto dxgi_capture = std::make_unique<DXGICapture>();
            if (!dxgi_capture->initialize(m_device.Get(), hmon))
            {
                LOG_ERROR("CaptureManager: desktop fallback capture initialization failed");
                return false;
            }
            m_capture_target_name = "Desktop";
            m_capture = std::move(dxgi_capture);
        }
        else
        {
            const std::string title = GameDetector::windowTitle(target_window);
            m_capture_target_name = title.empty() ? wgc_capture->targetName() : title;
            m_capture = std::move(wgc_capture);
        }
    }

    const int captured_width = m_capture ? m_capture->width() : monitor_width;
    const int captured_height = m_capture ? m_capture->height() : monitor_height;
    if (!has_width_override && captured_width > 0)
        actual_enc_config.width = captured_width;
    if (!has_height_override && captured_height > 0)
        actual_enc_config.height = captured_height;

    const int requested_width = actual_enc_config.width;
    const int requested_height = actual_enc_config.height;
    actual_enc_config.width = encoderSafeDimension(actual_enc_config.width);
    actual_enc_config.height = encoderSafeDimension(actual_enc_config.height);
    if (actual_enc_config.width != requested_width || actual_enc_config.height != requested_height)
    {
        LOG_INFO("CaptureManager: adjusted capture size for encoder alignment from " +
                 std::to_string(requested_width) + "x" + std::to_string(requested_height) +
                 " to " + std::to_string(actual_enc_config.width) + "x" +
                 std::to_string(actual_enc_config.height));
    }

    LOG_INFO("CaptureManager: encoder input size " +
             std::to_string(actual_enc_config.width) + "x" +
             std::to_string(actual_enc_config.height) + " from " +
             m_capture_source_name + " target '" + m_capture_target_name + "'");

    // ── Create encoder ───────────────────────────────────────

    const auto candidates = buildEncoderCandidates(
        encoderBackendFromConfig(actual_enc_config.backend),
        gpu_vendor,
        actual_enc_config.codec);

    for (const auto& candidate : candidates)
    {
        LOG_INFO("CaptureManager: trying encoder " + candidate.display_name +
                 " (" + candidate.encoder_name + ")");
        auto encoder = std::make_unique<FFmpegVideoEncoder>(candidate);
        if (encoder->initialize(m_device.Get(), actual_enc_config))
        {
            m_encoder = std::move(encoder);
            break;
        }
        LOG_WARNING("CaptureManager: encoder failed, trying next candidate: " +
                    candidate.display_name);
    }

    if (!m_encoder)
    {
        LOG_ERROR("CaptureManager: all video encoder candidates failed");
        return false;
    }

    m_width  = m_encoder->width();
    m_height = m_encoder->height();

    LOG_INFO("CaptureManager: initialized " +
             std::to_string(m_width) + "x" + std::to_string(m_height));
    return true;
}

// ── Start capture → encode → buffer pipeline ──────────────────

void CaptureManager::start(RingBuffer& buffer)
{
    if (!m_capture || !m_encoder)
    {
        LOG_ERROR("CaptureManager: start() called before initialize()");
        return;
    }

    // Wire up the frame callback: capture → encode → buffer
    m_capture->setFrameCallback(
        [this, &buffer](ID3D11Texture2D* texture, int64_t pts)
        {
            if (!m_encoder || !texture) return;

            D3D11_TEXTURE2D_DESC desc = {};
            texture->GetDesc(&desc);
            const int frame_width = encoderSafeDimension(static_cast<int>(desc.Width));
            const int frame_height = encoderSafeDimension(static_cast<int>(desc.Height));
            if (m_width > 0 && m_height > 0 &&
                (frame_width != m_width || frame_height != m_height))
            {
                if (!m_size_mismatch_warned)
                {
                    m_size_mismatch_warned = true;
                    LOG_WARNING("CaptureManager: capture target resized from encoder size " +
                                std::to_string(m_width) + "x" + std::to_string(m_height) +
                                " to " + std::to_string(desc.Width) + "x" +
                                std::to_string(desc.Height) +
                                "; restart capture to apply the new size");
                }
                return;
            }

            // 1) Submit texture to the selected FFmpeg encoder
            m_encoder->sendFrame(texture, pts);

            // 2) Collect encoded packets (encoders may emit multiple per submit)
            EncodedPacket pkt;
            while (m_encoder->receivePacket(pkt))
            {
                RingBufferPacket rbp;
                rbp.data.assign(pkt.data.begin(), pkt.data.end());
                rbp.pts = pkt.pts;
                rbp.dts = pkt.dts;
                rbp.duration = pkt.duration;
                rbp.is_keyframe = pkt.is_keyframe;

                buffer.push(std::move(rbp));
            }
        });

    m_running = true;
    m_capture->start();

    LOG_INFO("CaptureManager: pipeline started");
}

void CaptureManager::stop()
{
    m_running = false;

    if (m_capture)
        m_capture->stop();

    LOG_INFO("CaptureManager: stopped");
}
