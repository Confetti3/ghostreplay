#pragma once

#include <d3d11.h>
#include <Windows.h>
#include <functional>
#include <atomic>
#include <memory>

#include "capture/ICapture.h"

// DXGI Desktop Duplication capture backend.
// Captures a monitor using IDXGIOutputDuplication (DXGI 1.2).
//
// Pros: No WinRT dependency, works with raw D3D11/DXGI COM.
// Cons: May not capture fullscreen exclusive games; frame rate capped
//       at monitor refresh. This is the Phase 0 default.
//
// WGC (Windows Graphics Capture API) will replace this in Phase 1.
class DXGICapture : public ICapture
{
public:
    DXGICapture();
    ~DXGICapture() override;

    bool initialize(ID3D11Device* device, void* monitor) override;
    void start() override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    int width() const override;
    int height() const override;
    void setFrameCallback(FrameCallback callback) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    FrameCallback m_callback;
    std::atomic<bool> m_running{ false };
};
