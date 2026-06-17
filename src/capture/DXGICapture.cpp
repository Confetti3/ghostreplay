#include "capture/DXGICapture.h"
#include "util/Logger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <atomic>
#include <thread>
#include <chrono>

using Microsoft::WRL::ComPtr;
using namespace std::chrono_literals;

struct DXGICapture::Impl
{
    ID3D11Device* device = nullptr;

    ComPtr<IDXGIOutputDuplication> desk_dup;
    ComPtr<IDXGIOutput1> output1;

    int width = 0;
    int height = 0;
    HMONITOR hmon = nullptr;

    std::thread capture_thread;
    std::atomic<bool> capture_active{ false };

    FrameCallback callback;
    std::chrono::steady_clock::time_point start_time;

    void capture_loop();
    bool init_duplication(HMONITOR monitor);
};

bool DXGICapture::Impl::init_duplication(HMONITOR monitor)
{
    // Get DXGI factory
    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr))
    {
        LOG_ERROR("DXGI: CreateDXGIFactory1 failed, hr=0x" + std::to_string(hr));
        return false;
    }

    // Find the output matching the monitor
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi))
    {
        LOG_ERROR("DXGI: GetMonitorInfoW failed");
        return false;
    }

    ComPtr<IDXGIOutput> output;
    for (UINT i = 0; ; i++)
    {
        ComPtr<IDXGIAdapter1> adapter;
        if (FAILED(factory->EnumAdapters1(i, &adapter))) break;

        for (UINT j = 0; ; j++)
        {
            ComPtr<IDXGIOutput> out;
            if (FAILED(adapter->EnumOutputs(j, &out))) break;

            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(out->GetDesc(&desc)) &&
                wcscmp(desc.DeviceName, mi.szDevice) == 0)
            {
                output = out;
                break;
            }
        }
        if (output) break;
    }

    if (!output)
    {
        LOG_ERROR("DXGI: could not find output for monitor");
        return false;
    }

    // QI for IDXGIOutput1 to get DuplicateOutput
    hr = output.As(&output1);
    if (FAILED(hr))
    {
        LOG_ERROR("DXGI: IDXGIOutput1 not supported, hr=0x" + std::to_string(hr));
        return false;
    }

    // Create duplication
    hr = output1->DuplicateOutput(device, &desk_dup);
    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            LOG_ERROR("DXGI: DuplicateOutput failed — desktop not available "
                      "(fullscreen exclusive app may be running)");
        else if (hr == E_ACCESSDENIED)
            LOG_ERROR("DXGI: DuplicateOutput failed — access denied "
                      "(running as admin may help, or another app holds duplication)");
        else
            LOG_ERROR("DXGI: DuplicateOutput failed, hr=0x" + std::to_string(hr));
        return false;
    }

    // Get output description for dimensions
    DXGI_OUTPUT_DESC desc;
    output1->GetDesc(&desc);

    width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
    height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

    LOG_INFO("DXGI: duplication initialized for " +
             std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void DXGICapture::Impl::capture_loop()
{
    LOG_INFO("DXGI: capture loop started");

    capture_active = true;

    while (capture_active)
    {
        if (!desk_dup) break;

        DXGI_OUTDUPL_FRAME_INFO frame_info = {};
        ComPtr<IDXGIResource> desktop_resource;

        HRESULT hr = desk_dup->AcquireNextFrame(16, &frame_info, &desktop_resource);
        // 16ms timeout ≈ 60fps maximum polling rate

        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            continue; // No new frame yet
        }
        else if (hr == DXGI_ERROR_ACCESS_LOST)
        {
            LOG_WARNING("DXGI: access lost, attempting reacquire...");
            // In production, re-init duplication here
            std::this_thread::sleep_for(500ms);
            continue;
        }
        else if (FAILED(hr))
        {
            std::this_thread::sleep_for(100ms);
            continue;
        }

        // Skip frames that haven't changed
        if (frame_info.LastPresentTime.QuadPart == 0)
        {
            desk_dup->ReleaseFrame();
            continue;
        }

        // Get the D3D11 texture
        ComPtr<ID3D11Texture2D> texture;
        hr = desktop_resource.As(&texture);
        if (FAILED(hr))
        {
            LOG_WARNING("DXGI: failed to get texture from desktop resource");
            desk_dup->ReleaseFrame();
            continue;
        }

        // Compute PTS
        auto now = std::chrono::steady_clock::now();
        int64_t pts = std::chrono::duration_cast<std::chrono::microseconds>(
            now - start_time).count();

        // Invoke callback
        if (callback && texture)
        {
            callback(texture.Get(), pts);
        }

        desk_dup->ReleaseFrame();
    }

    LOG_INFO("DXGI: capture loop exited");
}

// ── PUBLIC INTERFACE ──────────────────────────────────────────

DXGICapture::DXGICapture() : m_impl(std::make_unique<Impl>()) {}
DXGICapture::~DXGICapture() { stop(); }

bool DXGICapture::initialize(ID3D11Device* device, void* monitor)
{
    auto& impl = *m_impl;
    impl.device = device;
    impl.hmon = static_cast<HMONITOR>(monitor);

    if (!impl.init_duplication(static_cast<HMONITOR>(monitor)))
        return false;

    LOG_INFO("DXGI: initialized");
    return true;
}

int DXGICapture::width() const
{
    return m_impl->width;
}

int DXGICapture::height() const
{
    return m_impl->height;
}

void DXGICapture::start()
{
    auto& impl = *m_impl;
    impl.start_time = std::chrono::steady_clock::now();
    m_running = true;
    impl.capture_thread = std::thread(&Impl::capture_loop, m_impl.get());
}

void DXGICapture::stop()
{
    auto& impl = *m_impl;
    impl.capture_active = false;

    if (impl.capture_thread.joinable())
        impl.capture_thread.join();

    m_running = false;
}

void DXGICapture::setFrameCallback(FrameCallback callback)
{
    m_callback = std::move(callback);
    m_impl->callback = m_callback;
}
