#pragma once

#include <d3d11.h>
#include <memory>
#include <vector>

#include "encode/EncoderConfig.h"
#include "encode/EncoderSelector.h"
#include "encode/IEncoder.h"

class FFmpegVideoEncoder : public IEncoder
{
public:
    explicit FFmpegVideoEncoder(EncoderCandidate candidate);
    ~FFmpegVideoEncoder() override;

    bool initialize(ID3D11Device* device, const EncoderConfig& config) override;

    bool sendFrame(ID3D11Texture2D* texture, int64_t pts) override;
    bool receivePacket(EncodedPacket& out) override;
    bool receivePacket(EncodedPacket& out, bool flush) override;

    std::vector<uint8_t> sequenceHeaders() const override;
    int inputFormat() const override { return m_input_format; }
    int width() const override { return m_width; }
    int height() const override { return m_height; }
    int framerateNum() const override { return m_fps_num; }
    int framerateDen() const override { return m_fps_den; }

    const EncoderCandidate& candidate() const { return m_candidate; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    EncoderCandidate m_candidate;

    int m_width = 0;
    int m_height = 0;
    int m_fps_num = 60;
    int m_fps_den = 1;
    int m_input_format = 0;
};
