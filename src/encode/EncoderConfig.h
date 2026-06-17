#pragma once

#include <cstdint>
#include <string>

struct EncoderConfig
{
    // Encoder selection
    enum class Codec { H264, HEVC } codec = Codec::H264;
    enum class Backend { Auto, NVENC, AMF, QSV, MF } backend = Backend::Auto;

    // Constant QP rate control where supported by the selected backend.
    // Lower = better quality, higher file size
    int qp_i = 23; // I-frame QP
    int qp_p = 23; // P-frame QP
    int qp_b = 23; // B-frame QP (NVENC defaults to P-only)

    // Resolution override (0 = use captured resolution)
    int width = 0;
    int height = 0;

    // Framerate (0 = use captured framerate)
    int fps_num = 60;
    int fps_den = 1;

    // GOP: IDR frame interval in frames
    // Fixed interval for predictable clip alignment
    // e.g. 2 seconds at 60fps = 120 frames
    int gop_length = 120;

    // Quality/speed preset: 1 (fastest) to 7 (slowest/best quality)
    // Default P4 is a good balance
    int preset = 4;

    // H.264 profile
    std::string profile = "high";

    // Additional options passed to the selected FFmpeg encoder as key=value pairs
    // e.g., "rc-lookahead=32", "no-scenecut=1", "b_adapt=0"
    std::string extra_opts;
};
