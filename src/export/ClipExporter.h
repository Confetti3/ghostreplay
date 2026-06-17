#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include "buffer/RingBuffer.h"
struct EncodedAudioPacket;

// Muxes H.264 + AAC packets into an MP4 container using FFmpeg libavformat.
// Handles Annex B → AVCC conversion for H.264 and audio extradata for AAC.
class ClipExporter
{
public:
    ClipExporter();
    ~ClipExporter();

    // ── Video-only initialise (backward compatible) ────────
    bool initialize(const std::string& output_path,
                    const std::vector<uint8_t>& sps_pps,
                    int width, int height,
                    int fps_num, int fps_den);

    // ── Video + audio initialise ──────────────────────────
    bool initialize(const std::string& output_path,
                    const std::vector<uint8_t>& sps_pps,
                    int width, int height,
                    int fps_num, int fps_den,
                    const std::vector<uint8_t>& audio_extradata,
                    int audio_sample_rate,
                    int audio_channels);

    // ── Write packets ─────────────────────────────────────
    bool writeVideoPackets(const std::vector<RingBufferPacket>& packets);
    bool writeAudioPackets(const std::vector<EncodedAudioPacket>& packets);

    // ── Finalise ──────────────────────────────────────────
    bool finalize();

    // Filename generation
    static std::string generateFilename(const std::string& output_dir,
                                         const std::string& game_name = "");

private:
    bool writeHeader();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

