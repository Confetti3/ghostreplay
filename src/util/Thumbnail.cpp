#include "util/Thumbnail.h"
#include "util/Logger.h"

#include <cstdint>
#include <vector>

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QtGlobal>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// ── Helper: RAII guard that calls ff_fn on scope exit ──────────
template <typename T, typename F>
struct ScopeGuard {
    T obj;
    F fn;
    ScopeGuard(T o, F f) : obj(o), fn(f) {}
    ~ScopeGuard() { if (obj && fn) fn(&obj); }
};

QPixmap ThumbnailExtractor::extract(const std::string& path, int thumb_w, int thumb_h)
{
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0)
    {
        LOG_WARNING(std::string("Thumbnail: cannot open ") + path);
        return {};
    }
    ScopeGuard<AVFormatContext*, void(*)(AVFormatContext**)> fmt_guard(fmt_ctx, [](AVFormatContext** p) { avformat_close_input(p); });

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
    {
        LOG_WARNING(std::string("Thumbnail: no stream info for ") + path);
        return {};
    }

    int video_idx = -1;
    const AVCodec* codec = nullptr;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i)
    {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_idx = static_cast<int>(i);
            codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            break;
        }
    }
    if (video_idx < 0 || !codec)
    {
        LOG_WARNING(std::string("Thumbnail: no video stream in ") + path);
        return {};
    }

    AVCodecContext* dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) return {};
    ScopeGuard<AVCodecContext*, void(*)(AVCodecContext**)> dec_guard(dec_ctx, [](AVCodecContext** p) { avcodec_free_context(p); });

    if (avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_idx]->codecpar) < 0) return {};
    if (avcodec_open2(dec_ctx, codec, nullptr) < 0) return {};

    // Seek to ~1 second into the clip
    int64_t seek_ts = av_rescale_q(1 * AV_TIME_BASE, {1, 1}, fmt_ctx->streams[video_idx]->time_base);
    av_seek_frame(fmt_ctx, video_idx, seek_ts, AVSEEK_FLAG_BACKWARD);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    ScopeGuard<AVPacket*, void(*)(AVPacket**)> pkt_guard(pkt, [](AVPacket** p) { av_packet_free(p); });
    ScopeGuard<AVFrame*, void(*)(AVFrame**)> frm_guard(frame, [](AVFrame** p) { av_frame_free(p); });

    QPixmap result;
    while (av_read_frame(fmt_ctx, pkt) >= 0)
    {
        if (pkt->stream_index != video_idx)
        {
            av_packet_unref(pkt);
            continue;
        }
        int ret = avcodec_send_packet(dec_ctx, pkt);
        av_packet_unref(pkt);
        if (ret < 0) continue;

        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN)) continue;
        if (ret < 0) break;

        // Got a frame — convert to RGB
        const double source_aspect = frame->height > 0
            ? static_cast<double>(frame->width) / static_cast<double>(frame->height)
            : 16.0 / 9.0;
        const double target_aspect = thumb_h > 0
            ? static_cast<double>(thumb_w) / static_cast<double>(thumb_h)
            : source_aspect;
        const int fit_w = target_aspect > source_aspect
            ? qMax(1, static_cast<int>(thumb_h * source_aspect))
            : thumb_w;
        const int fit_h = target_aspect > source_aspect
            ? thumb_h
            : qMax(1, static_cast<int>(thumb_w / source_aspect));

        SwsContext* sws = sws_getContext(
            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
            fit_w, fit_h, AV_PIX_FMT_RGB32,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!sws) break;

        QImage scaled(fit_w, fit_h, QImage::Format_RGB32);
        scaled.fill(0xff050607);
        uint8_t* dst_data[1] = { scaled.bits() };
        int dst_linesize[1] = { static_cast<int>(scaled.bytesPerLine()) };

        sws_scale(sws, frame->data, frame->linesize, 0, frame->height, dst_data, dst_linesize);
        sws_freeContext(sws);

        QImage canvas(thumb_w, thumb_h, QImage::Format_RGB32);
        canvas.fill(0xff050607);
        QPainter painter(&canvas);
        painter.drawImage((thumb_w - fit_w) / 2, (thumb_h - fit_h) / 2, scaled);
        painter.end();

        result = QPixmap::fromImage(canvas);
        break;
    }

    return result;
}
