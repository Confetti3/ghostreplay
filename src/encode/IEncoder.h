#pragma once

#include <d3d11.h>
#include <cstdint>
#include <vector>
#include "EncoderConfig.h"

struct EncodedPacket
{
    std::vector<uint8_t> data;
    int64_t pts;       // presentation timestamp, microseconds
    int64_t dts;       // decode timestamp, microseconds
    bool is_keyframe = false;
    int64_t duration = 0; // frame duration, microseconds
};

// Abstract interface for FFmpeg-backed video encoders.
class IEncoder
{
public:
    virtual ~IEncoder() = default;

    // Initialize encoder with shared D3D11 device.
    // Hardware backends consume captured D3D11 textures where supported.
    virtual bool initialize(ID3D11Device* device, const EncoderConfig& config) = 0;

    // Submit a D3D11 texture for encoding.
    // The texture must outlive the encoding operation.
    // Call receivePacket() repeatedly to collect output.
    virtual bool sendFrame(ID3D11Texture2D* texture, int64_t pts) = 0;

    // Receive an encoded packet, if available.
    // Returns false if no packet is ready.
    // Caller takes ownership of the returned data.
    virtual bool receivePacket(EncodedPacket& out) = 0;

    // Flush any buffered frames and return remaining packets.
    // After flush(), no more packets will be produced until new frames arrive.
    virtual bool receivePacket(EncodedPacket& out, bool flush) = 0;

    // Get the H.264 sequence headers (SPS + PPS).
    // Must be called after at least one encoded IDR frame.
    virtual std::vector<uint8_t> sequenceHeaders() const = 0;

    // Return the pixel format the selected encoder expects for input textures.
    // Typically DXGI_FORMAT_NV12 or DXGI_FORMAT_BGRA_UNORM.
    virtual int inputFormat() const = 0;

    // Return configured width / height.
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int framerateNum() const = 0;
    virtual int framerateDen() const = 0;
};
