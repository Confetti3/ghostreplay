#pragma once

#include <vector>
#include <cstdint>
#include <deque>
#include <mutex>

extern "C" {
struct AVCodecContext;
struct AVFrame;
}

// Minimal FFmpeg AAC encoder wrapper.
// Feed interleaved float PCM; collect encoded AAC packets.
class AudioEncoder
{
public:
    AudioEncoder();
    ~AudioEncoder();

    struct Packet {
        std::vector<uint8_t> data;
        int64_t pts_samples = 0;    // PTS in sample units
        int64_t duration_us = 0;
    };

    // Initialize AAC encoder. Returns false on failure.
    bool initialize(int sample_rate, int channels, int bitrate_kbps);

    // Feed interleaved float PCM (num_frames x channels).
    // Encoded packets are queued; call receive() to collect.
    void feed(const float* interleaved, int num_frames, int64_t pts_samples);

    // Retrieve a queued encoded packet. Returns false if none available.
    bool receive(Packet& out);

    // AAC codec extradata (for MP4 muxer).
    std::vector<uint8_t> codecConfig() const;

    // Encode any remaining buffered frames and return their packets.
    void flush();

private:
    AVCodecContext* m_ctx = nullptr;
    AVFrame*       m_frame = nullptr;

    std::deque<Packet> m_queue;
    std::mutex         m_mutex;

    int m_sample_rate = 48000;
    int m_channels    = 2;
    int m_frame_size  = 1024;       // AAC-LC frame size

    // Accumulator for partial WASAPI reads
    std::vector<float> m_pcm_buf;
    int64_t            m_pts_acc  = 0;   // PTS for next frame to encode
    int64_t            m_enc_counter = 0; // frames actually submitted
};

