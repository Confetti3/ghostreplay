#include "capture/WGCCapture.h"
#include "util/Logger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <roapi.h>
#include <shobjidl_core.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/base.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#pragma comment(lib, "windowsapp.lib")

using namespace std::chrono_literals;
namespace wgc = winrt::Windows::Graphics::Capture;
namespace wgd = winrt::Windows::Graphics::DirectX;
namespace wgd11 = winrt::Windows::Graphics::DirectX::Direct3D11;
namespace wf = winrt::Windows::Foundation;

namespace
{
bool initializeWinrt(RO_INIT_TYPE type)
{
    const HRESULT hr = RoInitialize(type);
    if (SUCCEEDED(hr) || hr == S_FALSE || hr == RPC_E_CHANGED_MODE)
        return true;

    LOG_ERROR("WGC: RoInitialize failed, hr=0x" + std::to_string(static_cast<unsigned int>(hr)));
    return false;
}

std::string hresultMessage(const char* prefix, HRESULT hr)
{
    return std::string(prefix) + ", hr=0x" + std::to_string(static_cast<unsigned int>(hr));
}
}

struct WGCCapture::Impl
{
    ID3D11Device* device = nullptr;
    wgd11::IDirect3DDevice wgc_device{ nullptr };
    wgc::GraphicsCaptureItem capture_item{ nullptr };
    wgc::Direct3D11CaptureFramePool frame_pool{ nullptr };
    wgc::GraphicsCaptureSession session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 frame_size{};
    std::atomic<int> target_width{ 0 };
    std::atomic<int> target_height{ 0 };

    std::thread capture_thread;
    std::atomic<bool> capture_active{ false };
    FrameCallback callback;
    std::chrono::steady_clock::time_point start_time;
    std::string target_name = "Desktop";

    bool createWinrtDevice();
    bool initFromCaptureItem(wgc::GraphicsCaptureItem item, std::string fallback_name);
    bool initMonitor(HMONITOR monitor);
    bool initWindow(HWND hwnd);
    bool initPicker(HWND owner);
    bool createFramePoolAndSession();
    void captureLoop();
};

bool WGCCapture::Impl::createWinrtDevice()
{
    winrt::com_ptr<IDXGIDevice> dxgi_device;
    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(dxgi_device.put()));
    if (FAILED(hr))
    {
        LOG_ERROR(hresultMessage("WGC: D3D11 device is not an IDXGIDevice", hr));
        return false;
    }

    winrt::com_ptr<IInspectable> inspectable_device;
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable_device.put());
    if (FAILED(hr))
    {
        LOG_ERROR(hresultMessage("WGC: CreateDirect3D11DeviceFromDXGIDevice failed", hr));
        return false;
    }

    wgc_device = inspectable_device.as<wgd11::IDirect3DDevice>();
    return true;
}

bool WGCCapture::Impl::initFromCaptureItem(wgc::GraphicsCaptureItem item, std::string fallback_name)
{
    if (!item)
    {
        LOG_WARNING("WGC: no capture item selected");
        return false;
    }

    if (!wgc::GraphicsCaptureSession::IsSupported())
    {
        LOG_ERROR("WGC: Windows Graphics Capture is not supported on this system");
        return false;
    }

    capture_item = std::move(item);
    frame_size = capture_item.Size();
    target_width = frame_size.Width;
    target_height = frame_size.Height;

    std::string display_name;
    try
    {
        display_name = winrt::to_string(capture_item.DisplayName());
    }
    catch (...)
    {
        display_name.clear();
    }
    target_name = display_name.empty() ? std::move(fallback_name) : display_name;

    if (frame_size.Width <= 0 || frame_size.Height <= 0)
    {
        LOG_ERROR("WGC: capture item has invalid size");
        return false;
    }

    LOG_INFO("WGC: capture target '" + target_name + "' " +
             std::to_string(frame_size.Width) + "x" +
             std::to_string(frame_size.Height));

    return createFramePoolAndSession();
}

bool WGCCapture::Impl::initMonitor(HMONITOR monitor)
{
    auto interop = winrt::get_activation_factory<wgc::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();

    winrt::com_ptr<IInspectable> item;
    HRESULT hr = interop->CreateForMonitor(monitor, winrt::guid_of<IInspectable>(), item.put_void());
    if (FAILED(hr))
    {
        LOG_ERROR(hresultMessage("WGC: CreateForMonitor failed", hr));
        return false;
    }

    return initFromCaptureItem(item.as<wgc::GraphicsCaptureItem>(), "Desktop");
}

bool WGCCapture::Impl::initWindow(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
    {
        LOG_WARNING("WGC: invalid window target");
        return false;
    }

    auto interop = winrt::get_activation_factory<wgc::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();

    winrt::com_ptr<IInspectable> item;
    HRESULT hr = interop->CreateForWindow(hwnd, winrt::guid_of<IInspectable>(), item.put_void());
    if (FAILED(hr))
    {
        LOG_ERROR(hresultMessage("WGC: CreateForWindow failed", hr));
        return false;
    }

    return initFromCaptureItem(item.as<wgc::GraphicsCaptureItem>(), "Window");
}

bool WGCCapture::Impl::initPicker(HWND owner)
{
    try
    {
        wgc::GraphicsCapturePicker picker;
        if (owner)
            picker.as<IInitializeWithWindow>()->Initialize(owner);

        auto operation = picker.PickSingleItemAsync();
        while (operation.Status() == wf::AsyncStatus::Started)
        {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            Sleep(10);
        }

        if (operation.Status() == wf::AsyncStatus::Canceled)
        {
            LOG_WARNING("WGC: capture picker was cancelled");
            return false;
        }
        if (operation.Status() == wf::AsyncStatus::Error)
        {
            LOG_ERROR("WGC: capture picker failed before returning an item");
            return false;
        }

        wgc::GraphicsCaptureItem item = operation.GetResults();
        return initFromCaptureItem(item, "Window");
    }
    catch (const winrt::hresult_error& e)
    {
        LOG_ERROR("WGC: capture picker failed, hr=0x" +
                  std::to_string(static_cast<unsigned int>(e.code())));
        return false;
    }
}

bool WGCCapture::Impl::createFramePoolAndSession()
{
    try
    {
        frame_pool = wgc::Direct3D11CaptureFramePool::CreateFreeThreaded(
            wgc_device,
            wgd::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            3,
            frame_size);

        session = frame_pool.CreateCaptureSession(capture_item);
        try
        {
            session.IsCursorCaptureEnabled(false);
        }
        catch (...)
        {
            LOG_WARNING("WGC: cursor capture control is unavailable on this Windows build");
        }
        return true;
    }
    catch (const winrt::hresult_error& e)
    {
        LOG_ERROR("WGC: frame pool/session creation failed, hr=0x" +
                  std::to_string(static_cast<unsigned int>(e.code())));
        return false;
    }
}

void WGCCapture::Impl::captureLoop()
{
    initializeWinrt(RO_INIT_MULTITHREADED);
    LOG_INFO("WGC: capture loop started");

    try
    {
        session.StartCapture();
        capture_active = true;

        while (capture_active)
        {
            auto frame = frame_pool.TryGetNextFrame();
            if (!frame)
            {
                std::this_thread::sleep_for(1ms);
                continue;
            }

            const auto content_size = frame.ContentSize();
            if (content_size.Width > 0 && content_size.Height > 0 &&
                (content_size.Width != frame_size.Width || content_size.Height != frame_size.Height))
            {
                frame_size = content_size;
                target_width = frame_size.Width;
                target_height = frame_size.Height;
                frame_pool.Recreate(
                    wgc_device,
                    wgd::DirectXPixelFormat::B8G8R8A8UIntNormalized,
                    3,
                    frame_size);
                LOG_INFO("WGC: capture target resized to " +
                         std::to_string(frame_size.Width) + "x" +
                         std::to_string(frame_size.Height));
                continue;
            }

            auto surface = frame.Surface();
            auto dxgi_access = surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();

            winrt::com_ptr<ID3D11Texture2D> texture;
            const HRESULT hr = dxgi_access->GetInterface(IID_PPV_ARGS(texture.put()));
            if (SUCCEEDED(hr) && texture && callback)
            {
                const auto now = std::chrono::steady_clock::now();
                const int64_t pts = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - start_time).count();
                callback(texture.get(), pts);
            }
        }
    }
    catch (const winrt::hresult_error& e)
    {
        LOG_ERROR("WGC: capture loop failed, hr=0x" +
                  std::to_string(static_cast<unsigned int>(e.code())));
    }

    try
    {
        if (session)
            session.Close();
        if (frame_pool)
            frame_pool.Close();
    }
    catch (...)
    {
    }

    LOG_INFO("WGC: capture loop exited");
}

WGCCapture::WGCCapture() : m_impl(std::make_unique<Impl>()) {}
WGCCapture::~WGCCapture() { stop(); }

bool WGCCapture::initialize(ID3D11Device* device, void* monitor)
{
    return initializeForMonitor(device, static_cast<HMONITOR>(monitor));
}

bool WGCCapture::initializeForMonitor(ID3D11Device* device, HMONITOR monitor)
{
    auto& impl = *m_impl;
    impl.device = device;
    if (!device || !monitor)
        return false;

    if (!initializeWinrt(RO_INIT_MULTITHREADED))
        return false;
    if (!impl.createWinrtDevice())
        return false;
    return impl.initMonitor(monitor);
}

bool WGCCapture::initializeForWindow(ID3D11Device* device, HWND hwnd)
{
    auto& impl = *m_impl;
    impl.device = device;
    if (!device || !hwnd)
        return false;

    if (!initializeWinrt(RO_INIT_MULTITHREADED))
        return false;
    if (!impl.createWinrtDevice())
        return false;
    return impl.initWindow(hwnd);
}

bool WGCCapture::initializeWithPicker(ID3D11Device* device, HWND owner)
{
    auto& impl = *m_impl;
    impl.device = device;
    if (!device)
        return false;

    if (!initializeWinrt(RO_INIT_SINGLETHREADED))
        return false;
    if (!impl.createWinrtDevice())
        return false;
    return impl.initPicker(owner);
}

void WGCCapture::start()
{
    auto& impl = *m_impl;
    impl.start_time = std::chrono::steady_clock::now();

    m_running = true;
    impl.capture_thread = std::thread(&Impl::captureLoop, m_impl.get());
}

void WGCCapture::stop()
{
    auto& impl = *m_impl;
    impl.capture_active = false;

    if (impl.capture_thread.joinable())
        impl.capture_thread.join();

    m_running = false;
}

int WGCCapture::width() const
{
    return m_impl->target_width.load();
}

int WGCCapture::height() const
{
    return m_impl->target_height.load();
}

void WGCCapture::setFrameCallback(FrameCallback callback)
{
    m_callback = std::move(callback);
    m_impl->callback = m_callback;
}

std::string WGCCapture::targetName() const
{
    return m_impl->target_name;
}
