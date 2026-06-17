#include "encode/FFmpegVideoEncoder.h"
#include "util/Logger.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/opt.h>
}

#include <d3d11.h>
#include <dxgi.h>
#include <dxgiformat.h>

#include <atomic>
#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

struct FFmpegVideoEncoder::Impl
{
    const AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
    AVBufferRef* hw_device_ctx = nullptr;
    AVFrame* hw_frame = nullptr;

    ID3D11Device* d3d_device = nullptr;

    std::vector<uint8_t> sps_pps;
    std::deque<EncodedPacket> output_queue;
    std::mutex queue_mutex;

    int width = 1920;
    int height = 1080;
    int fps_num = 60;
    int fps_den = 1;
    int input_format = DXGI_FORMAT_B8G8R8A8_UNORM;

    std::atomic<bool> initialized{ false };
    int64_t frame_count = 0;
};

namespace
{
void releaseD3D11Texture(void* opaque, uint8_t*)
{
    auto* texture = static_cast<ID3D11Texture2D*>(opaque);
    if (texture)
        texture->Release();
}

std::string avErrorString(int err)
{
    char errbuf[256];
    av_strerror(err, errbuf, sizeof(errbuf));
    return errbuf;
}

void setPrivateInt(AVCodecContext* ctx, const char* name, int value)
{
    if (ctx && ctx->priv_data)
        av_opt_set_int(ctx->priv_data, name, value, 0);
}

void setPrivateString(AVCodecContext* ctx, const char* name, const char* value)
{
    if (ctx && ctx->priv_data)
        av_opt_set(ctx->priv_data, name, value, 0);
}

std::string nvencPreset(int preset)
{
    const int clamped = std::max(1, std::min(7, preset));
    return "p" + std::to_string(clamped);
}

const char* amfPreset(int preset)
{
    if (preset <= 2)
        return "speed";
    if (preset >= 6)
        return "quality";
    return "balanced";
}

const char* qsvPreset(int preset)
{
    if (preset <= 2)
        return "veryfast";
    if (preset <= 3)
        return "faster";
    if (preset == 4)
        return "medium";
    if (preset <= 6)
        return "slow";
    return "veryslow";
}

void applyEncoderOptions(AVCodecContext* ctx, const EncoderCandidate& candidate, const EncoderConfig& config)
{
    const int gop = config.gop_length > 0 ? config.gop_length : 120;
    ctx->gop_size = gop;
    ctx->max_b_frames = 0;
    setPrivateInt(ctx, "g", gop);
    setPrivateInt(ctx, "keyint", gop);
    setPrivateInt(ctx, "bf", 0);
    setPrivateInt(ctx, "max_b_frames", 0);

    switch (candidate.backend)
    {
    case EncoderBackend::NVENC:
    {
        const std::string preset = nvencPreset(config.preset);
        setPrivateString(ctx, "rc", "constqp");
        setPrivateInt(ctx, "qp", config.qp_p);
        setPrivateInt(ctx, "cqp", config.qp_p);
        setPrivateString(ctx, "preset", preset.c_str());
        setPrivateString(ctx, "tune", "ll");
        setPrivateInt(ctx, "forced-idr", 1);
        setPrivateInt(ctx, "forced_idr", 1);
        setPrivateInt(ctx, "zerolatency", 1);
        setPrivateInt(ctx, "sc_threshold", 0);
        setPrivateInt(ctx, "rc-lookahead", 0);
        if (candidate.codec == EncoderConfig::Codec::H264)
            setPrivateString(ctx, "profile", "high");
        break;
    }
    case EncoderBackend::AMF:
        setPrivateString(ctx, "rc", "cqp");
        setPrivateInt(ctx, "qp_i", config.qp_i);
        setPrivateInt(ctx, "qp_p", config.qp_p);
        setPrivateInt(ctx, "qp_b", config.qp_b);
        setPrivateString(ctx, "usage", "lowlatency_high_quality");
        setPrivateString(ctx, "preset", amfPreset(config.preset));
        setPrivateInt(ctx, "latency", 1);
        setPrivateInt(ctx, "forced_idr", 1);
        if (candidate.codec == EncoderConfig::Codec::H264)
            setPrivateString(ctx, "profile", "high");
        break;
    case EncoderBackend::QSV:
        setPrivateString(ctx, "preset", qsvPreset(config.preset));
        setPrivateInt(ctx, "forced_idr", 1);
        setPrivateInt(ctx, "idr_interval", 1);
        setPrivateInt(ctx, "scenario", 7);
        if (candidate.codec == EncoderConfig::Codec::H264)
            setPrivateString(ctx, "profile", "high");
        break;
    case EncoderBackend::MF:
        setPrivateString(ctx, "rate_control", "quality");
        setPrivateInt(ctx, "quality", std::max(1, 100 - (config.qp_p * 2)));
        setPrivateInt(ctx, "hw_encoding", 0);
        setPrivateString(ctx, "scenario", "archive");
        break;
    case EncoderBackend::Auto:
    default:
        break;
    }
}
}

FFmpegVideoEncoder::FFmpegVideoEncoder(EncoderCandidate candidate)
    : m_impl(std::make_unique<Impl>())
    , m_candidate(std::move(candidate))
{
}

FFmpegVideoEncoder::~FFmpegVideoEncoder()
{
    if (m_impl->hw_frame)
        av_frame_free(&m_impl->hw_frame);
    if (m_impl->ctx)
        avcodec_free_context(&m_impl->ctx);
    if (m_impl->hw_device_ctx)
        av_buffer_unref(&m_impl->hw_device_ctx);
}

bool FFmpegVideoEncoder::initialize(ID3D11Device* device, const EncoderConfig& config)
{
    auto& impl = *m_impl;

    impl.d3d_device = device;
    impl.width = config.width > 0 ? config.width : 1920;
    impl.height = config.height > 0 ? config.height : 1080;
    impl.fps_num = config.fps_num > 0 ? config.fps_num : 60;
    impl.fps_den = config.fps_den > 0 ? config.fps_den : 1;

    impl.hw_device_ctx = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
    if (!impl.hw_device_ctx)
    {
        LOG_ERROR("Encoder: av_hwdevice_ctx_alloc failed for " + m_candidate.display_name);
        return false;
    }

    auto* hw_ctx = reinterpret_cast<AVHWDeviceContext*>(impl.hw_device_ctx->data);
    auto* d3d11_ctx = static_cast<AVD3D11VADeviceContext*>(hw_ctx->hwctx);

    d3d11_ctx->device = device;
    device->AddRef();

    ID3D11DeviceContext* dev_ctx = nullptr;
    device->GetImmediateContext(&dev_ctx);
    d3d11_ctx->device_context = dev_ctx;

    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&d3d11_ctx->video_device));
    if (FAILED(hr))
    {
        LOG_ERROR("Encoder: failed to get ID3D11VideoDevice for " + m_candidate.display_name);
        return false;
    }

    hr = dev_ctx->QueryInterface(IID_PPV_ARGS(&d3d11_ctx->video_context));
    if (FAILED(hr))
    {
        LOG_ERROR("Encoder: failed to get ID3D11VideoContext for " + m_candidate.display_name);
        return false;
    }

    int ret = av_hwdevice_ctx_init(impl.hw_device_ctx);
    if (ret < 0)
    {
        LOG_ERROR("Encoder: D3D11 hardware device context init failed for " +
                  m_candidate.display_name + ": " + avErrorString(ret));
        return false;
    }

    impl.codec = avcodec_find_encoder_by_name(m_candidate.encoder_name.c_str());
    if (!impl.codec)
    {
        LOG_ERROR("Encoder: FFmpeg encoder not found: " + m_candidate.encoder_name);
        return false;
    }

    impl.ctx = avcodec_alloc_context3(impl.codec);
    if (!impl.ctx)
    {
        LOG_ERROR("Encoder: avcodec_alloc_context3 failed for " + m_candidate.display_name);
        return false;
    }

    impl.ctx->width = impl.width;
    impl.ctx->height = impl.height;
    impl.ctx->time_base = { impl.fps_den, impl.fps_num };
    impl.ctx->framerate = { impl.fps_num, impl.fps_den };
    impl.ctx->pix_fmt = AV_PIX_FMT_D3D11;

    AVBufferRef* hw_frames_ref = av_hwframe_ctx_alloc(impl.hw_device_ctx);
    if (!hw_frames_ref)
    {
        LOG_ERROR("Encoder: av_hwframe_ctx_alloc failed for " + m_candidate.display_name);
        return false;
    }

    auto* frames_ctx = reinterpret_cast<AVHWFramesContext*>(hw_frames_ref->data);
    frames_ctx->format = AV_PIX_FMT_D3D11;
    frames_ctx->sw_format = AV_PIX_FMT_BGRA;
    frames_ctx->width = impl.width;
    frames_ctx->height = impl.height;
    frames_ctx->initial_pool_size = 4;

    ret = av_hwframe_ctx_init(hw_frames_ref);
    if (ret < 0)
    {
        LOG_ERROR("Encoder: hardware frames context init failed for " +
                  m_candidate.display_name + ": " + avErrorString(ret));
        av_buffer_unref(&hw_frames_ref);
        return false;
    }

    impl.ctx->hw_frames_ctx = hw_frames_ref;

    applyEncoderOptions(impl.ctx, m_candidate, config);

    ret = avcodec_open2(impl.ctx, impl.codec, nullptr);
    if (ret < 0)
    {
        LOG_ERROR("Encoder: avcodec_open2 failed for " + m_candidate.display_name +
                  " (" + m_candidate.encoder_name + "): " + avErrorString(ret));
        return false;
    }

    impl.hw_frame = av_frame_alloc();
    if (!impl.hw_frame)
    {
        LOG_ERROR("Encoder: av_frame_alloc failed for " + m_candidate.display_name);
        return false;
    }

    m_width = impl.width;
    m_height = impl.height;
    m_fps_num = impl.fps_num;
    m_fps_den = impl.fps_den;
    m_input_format = impl.input_format;

    impl.initialized = true;
    LOG_INFO("Encoder: initialized " + m_candidate.display_name + " (" +
             m_candidate.encoder_name + ", " + std::to_string(impl.width) + "x" +
             std::to_string(impl.height) + " @ " + std::to_string(impl.fps_num) +
             "fps, CQP=" + std::to_string(config.qp_p) + ")");
    if (m_candidate.compatibility_fallback)
    {
        LOG_WARNING("Encoder: using Windows compatibility fallback; performance and hardware use are driver controlled");
    }
    return true;
}

bool FFmpegVideoEncoder::sendFrame(ID3D11Texture2D* texture, int64_t pts)
{
    auto& impl = *m_impl;
    if (!impl.initialized || !texture)
        return false;

    av_frame_unref(impl.hw_frame);
    impl.hw_frame->format = AV_PIX_FMT_D3D11;
    impl.hw_frame->width = impl.width;
    impl.hw_frame->height = impl.height;
    impl.hw_frame->hw_frames_ctx = av_buffer_ref(impl.ctx->hw_frames_ctx);
    impl.hw_frame->data[0] = reinterpret_cast<uint8_t*>(texture);
    impl.hw_frame->data[1] = nullptr;
    texture->AddRef();
    impl.hw_frame->buf[0] = av_buffer_create(nullptr, 0, releaseD3D11Texture, texture, 0);
    if (!impl.hw_frame->hw_frames_ctx || !impl.hw_frame->buf[0])
    {
        LOG_WARNING("Encoder: failed to reference captured D3D11 texture");
        av_frame_unref(impl.hw_frame);
        return false;
    }

    impl.hw_frame->pts = pts / 10;

    int ret = avcodec_send_frame(impl.ctx, impl.hw_frame);
    if (ret < 0)
    {
        if (ret != AVERROR(EAGAIN))
        {
            LOG_WARNING("Encoder: avcodec_send_frame failed for " +
                        m_candidate.display_name + ": " + avErrorString(ret));
            return false;
        }
    }

    impl.frame_count++;

    while (true)
    {
        AVPacket* pkt = av_packet_alloc();
        if (!pkt)
            break;

        ret = avcodec_receive_packet(impl.ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            av_packet_free(&pkt);
            break;
        }
        if (ret < 0)
        {
            LOG_WARNING("Encoder: avcodec_receive_packet failed for " +
                        m_candidate.display_name + ": " + avErrorString(ret));
            av_packet_free(&pkt);
            break;
        }

        EncodedPacket out;
        out.data.assign(pkt->data, pkt->data + pkt->size);
        out.pts = pkt->pts * 10;
        out.dts = pkt->dts * 10;
        out.is_keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
        out.duration = pkt->duration * 10;

        if (out.is_keyframe && impl.sps_pps.empty())
        {
            impl.sps_pps.assign(pkt->data, pkt->data + pkt->size);
            LOG_INFO("Encoder: captured sequence headers (" +
                     std::to_string(impl.sps_pps.size()) + " bytes)");
        }

        {
            std::lock_guard<std::mutex> lock(impl.queue_mutex);
            impl.output_queue.push_back(std::move(out));
        }

        av_packet_free(&pkt);
    }

    return true;
}

bool FFmpegVideoEncoder::receivePacket(EncodedPacket& out)
{
    return receivePacket(out, false);
}

bool FFmpegVideoEncoder::receivePacket(EncodedPacket& out, bool /*flush*/)
{
    auto& impl = *m_impl;
    std::lock_guard<std::mutex> lock(impl.queue_mutex);
    if (impl.output_queue.empty())
        return false;

    out = std::move(impl.output_queue.front());
    impl.output_queue.pop_front();
    return true;
}

std::vector<uint8_t> FFmpegVideoEncoder::sequenceHeaders() const
{
    if (m_impl->ctx && m_impl->ctx->extradata && m_impl->ctx->extradata_size > 0)
    {
        return std::vector<uint8_t>(
            m_impl->ctx->extradata,
            m_impl->ctx->extradata + m_impl->ctx->extradata_size);
    }
    return m_impl->sps_pps;
}
