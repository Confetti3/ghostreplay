#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include <string>

#include "capture/ICapture.h"
#include "encode/IEncoder.h"
#include "encode/EncoderConfig.h"

// Creates and owns the shared D3D11 device.
// Selects the correct GPU adapter for the target monitor.
// Passes the device to capture and encoder backends.
class CaptureManager
{
public:
    CaptureManager();
    ~CaptureManager();

    // Initialize D3D11 and select the adapter driving the given monitor.
    // monitor_index: 0 = primary, 1 = secondary, etc.
    bool initialize(int monitor_index, const EncoderConfig& enc_config,
                    const std::string& capture_source = "desktop",
                    void* owner_window = nullptr,
                    bool allow_window_picker = false);

    // Start the capture → encode → buffer pipeline.
    void start(class RingBuffer& buffer);

    // Stop capture. Blocks until capture thread exits.
    void stop();

    // Accessors
    ID3D11Device* device() const { return m_device.Get(); }
    ID3D11DeviceContext* context() const { return m_context.Get(); }
    IEncoder* encoder() const { return m_encoder.get(); }
    int width() const { return m_width; }
    int height() const { return m_height; }
    std::string captureTargetName() const { return m_capture_target_name; }
    std::string captureSourceName() const { return m_capture_source_name; }

private:
    bool createDevice(int monitor_index);
    bool createEncoder(const EncoderConfig& config);
    std::string getMonitorName(HMONITOR hmon) const;
    void captureThread(RingBuffer& buffer);

    Microsoft::WRL::ComPtr<ID3D11Device>        m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGIFactory1>       m_dxgi_factory;

    std::unique_ptr<ICapture>  m_capture;
    std::unique_ptr<IEncoder>  m_encoder;

    int m_width = 0;
    int m_height = 0;
    std::string m_capture_target_name = "Desktop";
    std::string m_capture_source_name = "desktop";
    bool m_running = false;
    bool m_size_mismatch_warned = false;
};
