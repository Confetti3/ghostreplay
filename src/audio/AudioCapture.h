#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

// Encoded AAC audio packet (PTS in samples for FFmpeg timebase compatibility).
struct EncodedAudioPacket
{
    std::vector<uint8_t> data;
    int64_t pts_samples = 0;        // PTS in sample units (for timebase {1, sr})
    int64_t duration_us = 0;        // duration in microseconds
};

// WASAPI loopback audio capture → AAC encode pipeline.
// Owns the capture thread and encoded packet ring buffer.
class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    // Initialize WASAPI loopback + FFmpeg AAC encoder.
    // Returns false if the default audio device cannot be opened.
    bool initialize(const struct AudioConfig& config);

    // Start / stop the capture thread.
    void start();
    void stop();

    // Flush encoded packets whose PTS (in samples) >= window_start_samples.
    void flushWindow(int64_t window_start_us,
                     std::vector<EncodedAudioPacket>& out);

    // Ring buffer control
    void clear();

    // Accessors
    int sampleRate() const { return m_sample_rate; }
    int channels()   const { return m_channels; }
    size_t packetCount() const;

    // AAC extradata for MP4 muxer (AudioSpecificConfig).
    std::vector<uint8_t> codecConfig() const;

private:
    void captureThread();

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    int m_sample_rate = 48000;
    int m_channels    = 2;
};

