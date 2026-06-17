#pragma once

#include <d3d11.h>
#include <Windows.h>
#include <functional>
#include <atomic>
#include <memory>
#include <thread>
#include <string>

#include "capture/ICapture.h"

// Windows Graphics Capture API backend.
// Captures a monitor using the WinRT GraphicsCapture APIs.
//
// Requirements:
//   - Windows 10 1903+ (10.0.18362)
//   - A DispatcherQueue on the capture thread (created automatically)
struct WGCCapture : ICapture
{
    WGCCapture();
    ~WGCCapture() override;

    bool initialize(ID3D11Device* device, void* monitor) override;
    bool initializeForMonitor(ID3D11Device* device, HMONITOR monitor);
    bool initializeForWindow(ID3D11Device* device, HWND hwnd);
    bool initializeWithPicker(ID3D11Device* device, HWND owner);
    void start() override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    int width() const override;
    int height() const override;
    void setFrameCallback(FrameCallback callback) override;
    std::string targetName() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    FrameCallback m_callback;
    std::atomic<bool> m_running{ false };
};
