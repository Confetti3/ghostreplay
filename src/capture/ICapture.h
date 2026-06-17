#pragma once

#include <d3d11.h>
#include <functional>
#include <cstdint>

// Abstract interface for screen capture backends.
// Implementations: WGCCapture, DXGICapture (Phase 1).
class ICapture
{
public:
    virtual ~ICapture() = default;

    // Initialize the capture backend.
    // device: shared D3D11 device (must have BGRA support)
    // monitor: HMONITOR of the target display
    virtual bool initialize(ID3D11Device* device, void* monitor) = 0;

    // Start capturing frames.
    virtual void start() = 0;

    // Stop capturing frames gracefully.
    virtual void stop() = 0;

    // Returns true while capture is active.
    virtual bool isRunning() const = 0;

    // Captured frame dimensions after initialization.
    virtual int width() const = 0;
    virtual int height() const = 0;

    // Frame callback signature:
    // texture: captured ID3D11Texture2D (GPU memory, do not hold after return)
    // pts: presentation timestamp in microseconds
    using FrameCallback = std::function<void(ID3D11Texture2D* texture, int64_t pts)>;

    // Set the callback invoked for each captured frame.
    virtual void setFrameCallback(FrameCallback callback) = 0;
};
