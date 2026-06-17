#include "export/ClipExporter.h"
#include "buffer/RingBuffer.h"
#include "audio/AudioCapture.h"
#include "util/Logger.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
}

#include <memory>
#include <string>
#include <ctime>
#include <direct.h>

// ── Internal state ──────────────────────────────────────────────

struct ClipExporter::Impl
{
    AVFormatContext* fmt_ctx       = nullptr;
    AVStream*        video_stream  = nullptr;
    AVStream*        audio_stream  = nullptr;
    bool             header_written = false;
    bool             has_audio = false;

    // Video extradata (AVCC format)
    std::vector<uint8_t> video_extradata;

    int width  = 0;
    int height = 0;
    AVRational video_timebase = { 1, 90000 };

    // Audio parameters
    int audio_sample_rate = 0;
    int audio_channels    = 0;
    AVRational audio_timebase; // set to {1, sample_rate}
};

// ── Constructor / Destructor ────────────────────────────────────

ClipExporter::ClipExporter()
    : m_impl(std::make_unique<Impl>())
{}

ClipExporter::~ClipExporter()
{
    auto& impl = *m_impl;
    if (impl.fmt_ctx)
    {
        if (impl.header_written)
            av_write_trailer(impl.fmt_ctx);
        if (impl.fmt_ctx->pb)
            avio_closep(&impl.fmt_ctx->pb);
        avformat_free_context(impl.fmt_ctx);
    }
}

// ── Annex B → AVCC helpers (same as before) ────────────────────

static bool parseAnnexB_SPS_PPS(const std::vector<uint8_t>& annexb,
                                 std::vector<uint8_t>& sps,
                                 std::vector<uint8_t>& pps)
{
    const uint8_t* data = annexb.data();
    size_t size = annexb.size();
    size_t pos = 0;

    while (pos + 4 <= size)
    {
        // Find start code
        size_t scan = pos;
        while (scan + 3 < size &&
               !(data[scan] == 0 && data[scan+1] == 0 &&
                 (data[scan+2] == 1 || (data[scan+2] == 0 && data[scan+3] == 1))))
        {
            scan++;
        }
        if (scan + 3 >= size) break;

        size_t start_len = (data[scan+2] == 1) ? 3 : 4;
        scan += start_len;

        // Find next start code
        size_t nal_start = scan;
        while (scan + 3 < size &&
               !(data[scan] == 0 && data[scan+1] == 0 &&
                 (data[scan+2] == 1 || (data[scan+2] == 0 && scan+3 < size && data[scan+3] == 1))))
        {
            scan++;
        }
        size_t nal_end = std::min(scan, size);
        if (nal_end <= nal_start) break;

        uint8_t nal_type = data[nal_start] & 0x1F;
        if (nal_type == 7)
            sps.assign(data + nal_start, data + nal_end);
        else if (nal_type == 8)
            pps.assign(data + nal_start, data + nal_end);

        pos = nal_end;
    }

    return !sps.empty() && !pps.empty();
}

static void buildAVCCExtradata(const std::vector<uint8_t>& sps,
                                 const std::vector<uint8_t>& pps,
                                 std::vector<uint8_t>& out)
{
    out.clear();
    out.push_back(0x01);                    // version
    out.push_back(sps[1]);                  // profile
    out.push_back(sps[2]);                  // compatibility
    out.push_back(sps[3]);                  // level
    out.push_back(0xFF);                    // 6 bits reserved + 2 bits NAL size = 0xFF
    out.push_back(0xE1);                    // 3 bits reserved + 5 bits SPS count = 0xE1

    uint16_t sps_size = static_cast<uint16_t>(sps.size());
    out.push_back(static_cast<uint8_t>(sps_size >> 8));
    out.push_back(static_cast<uint8_t>(sps_size & 0xFF));
    out.insert(out.end(), sps.begin(), sps.end());

    out.push_back(0x01);                    // PPS count = 1
    uint16_t pps_size = static_cast<uint16_t>(pps.size());
    out.push_back(static_cast<uint8_t>(pps_size >> 8));
    out.push_back(static_cast<uint8_t>(pps_size & 0xFF));
    out.insert(out.end(), pps.begin(), pps.end());
}

// ── Annex B → AVCC NAL conversion (per-frame) ──────────────────

static std::vector<uint8_t> annexbToAVCC(const uint8_t* data, size_t size)
{
    std::vector<uint8_t> out;
    const uint8_t* src = data;
    size_t pos = 0;

    while (pos + 4 <= size)
    {
        size_t scan = pos;
        while (scan + 3 < size &&
               !(src[scan] == 0 && src[scan+1] == 0 &&
                 (src[scan+2] == 1 || (src[scan+2] == 0 && src[scan+3] == 1))))
        {
            scan++;
        }
        if (scan + 3 >= size) break;

        size_t start_len = (src[scan+2] == 1) ? 3 : 4;
        scan += start_len;

        size_t nal_start = scan;
        while (scan + 3 < size &&
               !(src[scan] == 0 && src[scan+1] == 0 &&
                 (src[scan+2] == 1 || (src[scan+2] == 0 && scan+3 < size && src[scan+3] == 1))))
        {
            scan++;
        }
        size_t nal_end = std::min(scan, size);
        if (nal_end <= nal_start) break;

        uint32_t nal_len = static_cast<uint32_t>(nal_end - nal_start);
        out.push_back(static_cast<uint8_t>(nal_len >> 24));
        out.push_back(static_cast<uint8_t>(nal_len >> 16));
        out.push_back(static_cast<uint8_t>(nal_len >> 8));
        out.push_back(static_cast<uint8_t>(nal_len & 0xFF));
        out.insert(out.end(), src + nal_start, src + nal_end);

        pos = nal_end;
    }
    return out;
}

// ── Video-only initialise ──────────────────────────────────────

bool ClipExporter::initialize(const std::string& output_path,
                               const std::vector<uint8_t>& sps_pps,
                               int width, int height,
                               int fps_num, int fps_den)
{
    return initialize(output_path, sps_pps, width, height, fps_num, fps_den,
                       {}, 0, 0);
}

// ── Video + audio initialise ───────────────────────────────────

bool ClipExporter::initialize(const std::string& output_path,
                               const std::vector<uint8_t>& sps_pps,
                               int width, int height,
                               int fps_num, int fps_den,
                               const std::vector<uint8_t>& audio_extra,
                               int audio_sample_rate,
                               int audio_channels)
{
    auto& impl = *m_impl;

    impl.width  = width;
    impl.height = height;

    // Ensure output directory exists
    auto sep = output_path.find_last_of("\\/");
    if (sep != std::string::npos)
    {
        std::string dir = output_path.substr(0, sep);
        _mkdir(dir.c_str());
    }

    // ── Create output context ────────────────────────────────
    int ret = avformat_alloc_output_context2(&impl.fmt_ctx, nullptr,
                                              "mp4", output_path.c_str());
    if (ret < 0 || !impl.fmt_ctx)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));
        LOG_ERROR("ClipExporter: avformat_alloc_output_context2: " + std::string(err));
        return false;
    }

    // ── Video stream ────────────────────────────────────────
    impl.video_stream = avformat_new_stream(impl.fmt_ctx, nullptr);
    if (!impl.video_stream)
    {
        LOG_ERROR("ClipExporter: avformat_new_stream (video) failed");
        return false;
    }

    AVCodecParameters* vpar = impl.video_stream->codecpar;
    vpar->codec_type = AVMEDIA_TYPE_VIDEO;
    vpar->codec_id   = AV_CODEC_ID_H264;
    vpar->width      = width;
    vpar->height     = height;
    vpar->bit_rate   = 0;

    impl.video_stream->time_base       = impl.video_timebase;
    impl.video_stream->avg_frame_rate  = { fps_num, fps_den };

    // ── Video extradata (SPS/PPS Annex B → AVCC) ────────────
    if (!sps_pps.empty())
    {
        std::vector<uint8_t> sps, pps;
        if (parseAnnexB_SPS_PPS(sps_pps, sps, pps))
        {
            buildAVCCExtradata(sps, pps, impl.video_extradata);

            vpar->extradata = reinterpret_cast<uint8_t*>(
                av_mallocz(impl.video_extradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
            if (!vpar->extradata)
            {
                LOG_ERROR("ClipExporter: video extradata allocation failed");
                return false;
            }

            vpar->extradata_size = static_cast<int>(impl.video_extradata.size());
            memcpy(vpar->extradata, impl.video_extradata.data(), impl.video_extradata.size());
            LOG_INFO("ClipExporter: AVCC extradata SPS=" +
                     std::to_string(sps.size()) + " PPS=" +
                     std::to_string(pps.size()));
        }
        else
        {
            LOG_WARNING("ClipExporter: SPS/PPS parse failed");
        }
    }

    // ── Audio stream (if requested) ─────────────────────────
    if (audio_sample_rate > 0 && audio_channels > 0)
    {
        impl.audio_stream = avformat_new_stream(impl.fmt_ctx, nullptr);
        if (!impl.audio_stream)
        {
            LOG_WARNING("ClipExporter: avformat_new_stream (audio) failed");
        }
        else
        {
            impl.has_audio          = true;
            impl.audio_sample_rate  = audio_sample_rate;
            impl.audio_channels     = audio_channels;
            impl.audio_timebase     = { 1, audio_sample_rate };

            AVCodecParameters* apar = impl.audio_stream->codecpar;
            apar->codec_type  = AVMEDIA_TYPE_AUDIO;
            apar->codec_id    = AV_CODEC_ID_AAC;
            apar->sample_rate = audio_sample_rate;
            apar->frame_size  = 1024;
            av_channel_layout_default(&apar->ch_layout, audio_channels);

            impl.audio_stream->time_base = impl.audio_timebase;

            if (!audio_extra.empty())
            {
                // We need to store the extradata somewhere that survives.
                // Use a static/heap. Since we only need it for open, we can
                // allocate via av_malloc and let FFmpeg own it.
                apar->extradata = reinterpret_cast<uint8_t*>(
                    av_mallocz(audio_extra.size() + AV_INPUT_BUFFER_PADDING_SIZE));
                if (!apar->extradata)
                {
                    LOG_WARNING("ClipExporter: audio extradata allocation failed");
                }
                else
                {
                apar->extradata_size = static_cast<int>(audio_extra.size());
                memcpy(apar->extradata, audio_extra.data(), audio_extra.size());
                }
            }

            LOG_INFO("ClipExporter: audio stream " +
                     std::to_string(audio_sample_rate) + "Hz " +
                     std::to_string(audio_channels) + "ch");
        }
    }

    // ── Open output file ─────────────────────────────────────
    ret = avio_open(&impl.fmt_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));
        LOG_ERROR("ClipExporter: avio_open: " + std::string(err));
        return false;
    }

    LOG_INFO("ClipExporter: output opened: " + output_path);
    return true;
}

// ── Write header ────────────────────────────────────────────────

bool ClipExporter::writeHeader()
{
    auto& impl = *m_impl;
    if (impl.header_written) return true;

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "movflags", "faststart", 0);

    int ret = avformat_write_header(impl.fmt_ctx, &opts);
    av_dict_free(&opts);

    if (ret < 0)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));
        LOG_ERROR("ClipExporter: avformat_write_header: " + std::string(err));
        return false;
    }

    impl.header_written = true;
    LOG_INFO("ClipExporter: header written");
    return true;
}

// ── Write video packets ────────────────────────────────────────

bool ClipExporter::writeVideoPackets(const std::vector<RingBufferPacket>& packets)
{
    auto& impl = *m_impl;
    if (!impl.fmt_ctx || !impl.video_stream) return false;

    if (!writeHeader()) return false;

    for (const auto& pkt : packets)
    {
        AVPacket avpkt = {};
        avpkt.stream_index = impl.video_stream->index;
        avpkt.flags = pkt.is_keyframe ? AV_PKT_FLAG_KEY : 0;

        // PTS conversion: microseconds → timebase
        AVRational us_tb = { 1, 1'000'000 };
        avpkt.pts = av_rescale_q(pkt.pts, us_tb, impl.video_timebase);
        avpkt.dts = av_rescale_q(pkt.dts, us_tb, impl.video_timebase);
        avpkt.duration = av_rescale_q(pkt.duration, us_tb, impl.video_timebase);

        // Annex B → AVCC
        std::vector<uint8_t> avcc = annexbToAVCC(pkt.data.data(), pkt.data.size());

        if (!avcc.empty())
        {
            avpkt.data = avcc.data();
            avpkt.size = static_cast<int>(avcc.size());

            int ret = av_interleaved_write_frame(impl.fmt_ctx, &avpkt);
            if (ret < 0)
            {
                char err[256];
                av_strerror(ret, err, sizeof(err));
                LOG_ERROR("ClipExporter: video write: " + std::string(err));
                return false;
            }
        }
        else
        {
            avpkt.data = const_cast<uint8_t*>(pkt.data.data());
            avpkt.size = static_cast<int>(pkt.data.size());

            int ret = av_interleaved_write_frame(impl.fmt_ctx, &avpkt);
            if (ret < 0)
            {
                char err[256];
                av_strerror(ret, err, sizeof(err));
                LOG_ERROR("ClipExporter: video write (raw): " + std::string(err));
                return false;
            }
        }
    }

    return true;
}

// ── Write audio packets ────────────────────────────────────────

bool ClipExporter::writeAudioPackets(const std::vector<EncodedAudioPacket>& packets)
{
    auto& impl = *m_impl;
    if (!impl.fmt_ctx || !impl.audio_stream || !impl.has_audio) return false;

    if (!writeHeader()) return false;

    for (const auto& apkt : packets)
    {
        AVPacket avpkt = {};
        avpkt.data         = const_cast<uint8_t*>(apkt.data.data());
        avpkt.size         = static_cast<int>(apkt.data.size());
        avpkt.stream_index = impl.audio_stream->index;
        avpkt.flags        = 0;

        // Audio PTS is already in samples (matches audio timebase)
        avpkt.pts = apkt.pts_samples;
        avpkt.dts = apkt.pts_samples;
        avpkt.duration = apkt.duration_us * impl.audio_sample_rate / 1'000'000LL;

        int ret = av_interleaved_write_frame(impl.fmt_ctx, &avpkt);
        if (ret < 0)
        {
            char err[256];
            av_strerror(ret, err, sizeof(err));
            LOG_ERROR("ClipExporter: audio write: " + std::string(err));
            return false;
        }
    }

    return true;
}

// ── Finalise ────────────────────────────────────────────────────

bool ClipExporter::finalize()
{
    auto& impl = *m_impl;
    if (!impl.fmt_ctx) return false;

    if (impl.header_written)
    {
        int ret = av_write_trailer(impl.fmt_ctx);
        if (ret < 0)
        {
            char err[256];
            av_strerror(ret, err, sizeof(err));
            LOG_ERROR("ClipExporter: av_write_trailer: " + std::string(err));
            return false;
        }
        LOG_INFO("ClipExporter: trailer written");
        impl.header_written = false;
    }

    avio_closep(&impl.fmt_ctx->pb);
    LOG_INFO("ClipExporter: file closed");
    return true;
}

// ── Filename generation ────────────────────────────────────────

std::string ClipExporter::generateFilename(const std::string& output_dir,
                                            const std::string& game_name)
{
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);

    char tsbuf[64];
    std::strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%d_%H-%M-%S", &tm);

    std::string path = output_dir;
    if (!path.empty() && path.back() != '\\' && path.back() != '/')
        path += '\\';

    // Prefix: game name (if provided) or fallback to "GhostReplay"
    std::string prefix = game_name.empty() ? "GhostReplay" : game_name;

    // Sanitise prefix for filename safety
    // (reuse logic; since we don't want circular deps, do inline)
    for (char& c : prefix)
    {
        if (c == '\\' || c == '/' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '-';
    }

    path += prefix;
    path += "_";
    path += tsbuf;
    path += ".mp4";
    return path;
}
