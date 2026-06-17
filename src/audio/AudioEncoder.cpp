extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include "audio/AudioEncoder.h"
#include "util/Logger.h"

#include <cstring>

static constexpr int AAC_FRAME_SIZE = 1024; // AAC-LC

AudioEncoder::AudioEncoder()  = default;
AudioEncoder::~AudioEncoder()
{
    if (m_frame) av_frame_free(&m_frame);
    if (m_ctx)   avcodec_free_context(&m_ctx);
}

bool AudioEncoder::initialize(int sample_rate, int channels, int bitrate_kbps)
{
    m_sample_rate = sample_rate;
    m_channels    = channels;
    m_frame_size  = AAC_FRAME_SIZE;

    const AVCodec* codec = avcodec_find_encoder_by_name("aac");
    if (!codec)
    {
        LOG_ERROR("AudioEncoder: AAC encoder not found in FFmpeg build");
        return false;
    }

    m_ctx = avcodec_alloc_context3(codec);
    if (!m_ctx)
    {
        LOG_ERROR("AudioEncoder: avcodec_alloc_context3 failed");
        return false;
    }

    m_ctx->sample_rate    = sample_rate;
    m_ctx->bit_rate       = bitrate_kbps * 1000;
    m_ctx->sample_fmt     = AV_SAMPLE_FMT_FLTP;
    m_ctx->time_base      = { 1, sample_rate };
    av_channel_layout_default(&m_ctx->ch_layout, channels);

    if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
        m_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    int ret = avcodec_open2(m_ctx, codec, nullptr);
    if (ret < 0)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));
        LOG_ERROR("AudioEncoder: avcodec_open2 failed: " + std::string(err));
        avcodec_free_context(&m_ctx);
        m_ctx = nullptr;
        return false;
    }

    // Allocate reusable frame
    m_frame = av_frame_alloc();
    m_frame->nb_samples   = AAC_FRAME_SIZE;
    m_frame->format       = AV_SAMPLE_FMT_FLTP;
    av_channel_layout_copy(&m_frame->ch_layout, &m_ctx->ch_layout);
    ret = av_frame_get_buffer(m_frame, 0);
    if (ret < 0)
    {
        LOG_ERROR("AudioEncoder: av_frame_get_buffer failed");
        av_frame_free(&m_frame);
        m_frame = nullptr;
        return false;
    }

    LOG_INFO("AudioEncoder: AAC encoder ready " +
             std::to_string(sample_rate) + "Hz " +
             std::to_string(channels) + "ch " +
             std::to_string(bitrate_kbps) + "kbps");
    return true;
}

void AudioEncoder::feed(const float* interleaved, int num_frames, int64_t pts_samples)
{
    if (!m_ctx || !m_frame) return;

    // Deinterleave: interleaved float → planar float channels
    for (int ch = 0; ch < m_channels; ++ch)
    {
        float* dst = reinterpret_cast<float*>(m_frame->data[ch]);
        for (int i = 0; i < num_frames; ++i)
            dst[i] = interleaved[i * m_channels + ch];
    }

    m_frame->pts       = pts_samples;
    m_frame->nb_samples = num_frames;

    int ret = avcodec_send_frame(m_ctx, m_frame);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));
        LOG_WARNING("AudioEncoder: avcodec_send_frame: " + std::string(err));
        return;
    }

    // Drain output
    AVPacket* pkt = av_packet_alloc();
    while (avcodec_receive_packet(m_ctx, pkt) == 0)
    {
        Packet out;
        out.data.assign(pkt->data, pkt->data + pkt->size);

        // FFmpeg audio PTS is in samples, matches timebase {1, sr}
        out.pts_samples = pkt->pts;

        int64_t pkt_dur_samples = (pkt->duration > 0 && pkt->duration <= m_frame_size * 4)
            ? pkt->duration
            : static_cast<int64_t>(m_frame_size);

        out.duration_us = pkt_dur_samples * 1'000'000LL / m_sample_rate;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push_back(std::move(out));
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}

bool AudioEncoder::receive(Packet& out)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) return false;
    out = std::move(m_queue.front());
    m_queue.pop_front();
    return true;
}

std::vector<uint8_t> AudioEncoder::codecConfig() const
{
    std::vector<uint8_t> r;
    if (m_ctx && m_ctx->extradata && m_ctx->extradata_size > 0)
        r.assign(m_ctx->extradata, m_ctx->extradata + m_ctx->extradata_size);
    return r;
}

void AudioEncoder::flush()
{
    if (!m_ctx) return;
    avcodec_send_frame(m_ctx, nullptr); // flush

    AVPacket* pkt = av_packet_alloc();
    while (avcodec_receive_packet(m_ctx, pkt) == 0)
    {
        Packet out;
        out.data.assign(pkt->data, pkt->data + pkt->size);
        out.pts_samples = pkt->pts;
        out.duration_us = AAC_FRAME_SIZE * 1'000'000LL / m_sample_rate;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push_back(std::move(out));
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}
