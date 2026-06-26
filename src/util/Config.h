#pragma once

#include <string>
#include <cstdint>

// ── Audio capture / encode configuration ────────────────────────

struct AudioConfig
{
    bool   enabled     = true;
    std::string device_id;                      // WASAPI device ID, empty = default output
    int    channels    = 2;                     // 1 = mono, 2 = stereo
    int    sample_rate = 48000;                 // 44100, 48000, 96000
    int    bitrate_kbps = 192;                  // AAC target bitrate
    std::string codec  = "aac";                 // aac only for Phase 1
};

// ── Runtime configuration ───────────────────────────────────────
// Loaded from ghostreplay.json; defaults shown below.

class Config
{
public:
    // Capture
    int monitor_index   = 0;
    int capture_width   = 0;                // 0 = use native (DXGI will downscale in encoder)
    int capture_height  = 0;
    std::string capture_source = "desktop"; // desktop | window | game

    // Replay buffer
    int replay_duration_sec = 300;          // 5 minutes
    int fps                 = 60;

    // Encoder
    std::string codec     = "h264";         // h264 | hevc
    std::string encoder_backend = "auto";   // auto | nvenc | amf | qsv | mf
    int cqp               = 23;             // encoder quality (0-51)
    int keyframe_interval = 120;            // IDR every N frames
    int preset            = 4;              // 1 (fastest) - 7 (best)

    // Audio
    AudioConfig audio;

    // Hotkey
    struct Hotkey {
        unsigned int modifiers = 1;         // MOD_ALT
        unsigned int vk        = 121;       // VK_F10
    } hotkey;

    // Output
    std::string output_dir = "clips";
    bool launch_at_startup = false;          // Register in Windows startup
    bool start_minimized   = false;          // Launch minimized to tray (internal/command-line only)

    // Logging
    std::string log_file  = "ghostreplay.log";
    int log_level         = 0;              // 0=debug  1=info  2=warn  3=error

    // ── I/O ──────────────────────────────────────────────────
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    void parseArgs(int argc, char* argv[]);

    // ── Windows startup registry toggle ──────────────────────
    static bool setStartupEnabled(bool enable);
    static bool isStartupEnabled();
};
