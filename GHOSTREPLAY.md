# GhostReplay — Historical Project Document

> This document is the original architecture and roadmap note. For current build,
> packaging, beta limitations, and release checks, see `README.md` and `docs/`.

> A lightweight, open-source instant replay tool for Windows with Nvidia, AMD, Intel, and Windows compatibility encoding paths.  
> Designed to replicate the low-overhead feel of ShadowPlay (circa 2016) with a clean UI  
> and a long-term roadmap toward a full clip management and editing suite.

---

## Design Goals

- Near-zero FPS impact during gameplay where vendor hardware encoding is available
- Always-on background operation, invisible until needed
- Single hotkey saves the last N minutes — no manual start/stop
- Own the full stack (no libobs dependency) for maximum control and minimal overhead
- Ship as open source under MIT or Apache 2.0 (rules out GPL dependencies like libobs)

---

## Technology Stack

| Concern | Choice | Rationale |
|---|---|---|
| Language | C++20 | Performance, Windows SDK access, you already know it |
| Build | CMake 3.25+ | You already use it |
| UI framework | Qt 6 | You already know it; handles tray, windows, hotkeys portably |
| Screen capture | Windows Graphics Capture API (WGC) | Modern, simple, window or monitor capture |
| Capture fallback | DXGI Desktop Duplication | DX9 game compatibility |
| Video encoding | FFmpeg (libavcodec) with NVENC / AMF / Quick Sync / Media Foundation | Vendor hardware encoders without SDK-specific pipelines |
| Audio capture | WASAPI loopback | Low-latency system audio, no driver needed |
| Muxing | FFmpeg (libavformat) | MP4/MKV output |
| Playback preview | libmpv | Embeds cleanly in Qt, built on FFmpeg |
| Compression export | FFmpeg | Re-encode with configurable CRF |

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   CaptureManager                    │
│  ┌──────────────────┐   ┌─────────────────────────┐ │
│  │  WGCCapture      │   │  DXGICapture (fallback)  │ │
│  │  ID3D11Texture2D │   │  ID3D11Texture2D         │ │
│  └────────┬─────────┘   └──────────┬──────────────┘ │
└───────────┼──────────────────────── ┼───────────────┘
            │  GPU texture (no copy)  │
            ▼                         ▼
┌─────────────────────────────────────────────────────┐
│                  FFmpegVideoEncoder                   │
│  D3D11 hardware frames → vendor encoder or MF         │
│  Outputs: AVPacket (encoded H.264/HEVC on CPU)       │
└───────────────────────────┬─────────────────────────┘
                            │
            ┌───────────────┴───────────────┐
            ▼                               ▼
┌───────────────────┐           ┌───────────────────────┐
│   VideoRingBuffer │           │   AudioRingBuffer      │
│   (AVPacket deque)│           │   (PCM/AAC packet deque│
│   keyframe-aware  │           │   timestamped)         │
└─────────┬─────────┘           └──────────┬────────────┘
          │  on hotkey                      │
          └──────────────┬─────────────────┘
                         ▼
               ┌──────────────────┐
               │   ClipExporter   │
               │  FFmpeg mux →    │
               │  MP4 to disk     │
               └──────────────────┘
```

The critical hardware path keeps captured frames on the GPU where the selected FFmpeg backend supports D3D11 frames.

---

## Directory Structure

```
ghostreplay/
├── CMakeLists.txt
├── README.md
├── ROADMAP.md                        ← this file
├── LICENSE
│
├── src/
│   ├── main.cpp                      ← entry point, Qt app init
│   │
│   ├── capture/
│   │   ├── ICapture.h                ← abstract interface
│   │   ├── CaptureManager.h/.cpp     ← selects backend, owns frame loop
│   │   ├── WGCCapture.h/.cpp         ← Windows Graphics Capture API
│   │   └── DXGICapture.h/.cpp        ← DXGI Desktop Duplication fallback
│   │
│   ├── encode/
│   │   ├── IEncoder.h                ← abstract interface
│   │   ├── FFmpegVideoEncoder.h/.cpp ← FFmpeg encoder backend, D3D11 interop
│   │   └── EncoderConfig.h           ← codec, bitrate, CQP, keyframe interval
│   │
│   ├── buffer/
│   │   ├── RingBuffer.h/.cpp         ← circular AVPacket store
│   │   └── KeyframeIndex.h/.cpp      ← tracks IDR positions for clean cuts
│   │
│   ├── audio/
│   │   ├── WASAPICapture.h/.cpp      ← loopback audio capture thread
│   │   └── AudioEncoder.h/.cpp       ← FFmpeg AAC encoding
│   │
│   ├── hotkey/
│   │   └── HotkeyManager.h/.cpp      ← Win32 RegisterHotKey wrapper
│   │
│   ├── export/
│   │   ├── ClipExporter.h/.cpp       ← flushes ring buffer → muxed MP4
│   │   └── ReencodeExporter.h/.cpp   ← post-process compress (Phase 3)
│   │
│   ├── ui/                           ← Qt (Phase 2 onward)
│   │   ├── TrayIcon.h/.cpp
│   │   ├── SettingsWindow.h/.cpp
│   │   ├── ClipLibrary.h/.cpp
│   │   └── ClipEditor.h/.cpp
│   │
│   └── util/
│       ├── Logger.h/.cpp             ← spdlog or simple file logger
│       ├── Config.h/.cpp             ← JSON settings persistence
│       └── GameDetector.h/.cpp       ← process monitor (Phase 2)
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_ringbuffer.cpp
│   ├── test_encoder.cpp
│   └── test_capture.cpp
│
└── third_party/
    ├── ffmpeg/                       ← prebuilt Windows libs (gpl-free build)
    └── catch2/                       ← or googletest, for unit tests
```

---

## Build Requirements

**System**
- Windows 10 version 1903 or later (for WGC)
- Supported encoder: Nvidia NVENC, AMD AMF, Intel Quick Sync, or Windows Media Foundation
- Recent GPU/display driver

**Toolchain**
- MSVC 2022 or Clang-cl (C++20)
- CMake 3.25+
- Windows SDK 10.0.19041.0 or later (for WGC headers)
- Qt 6.5+ (for Phase 2 UI)

**Libraries**
- FFmpeg 6.x or 7.x — shared build, LGPL-compatible (no GPL encoders enabled)
  - libavcodec, libavformat, libavutil, libswresample, libswscale
- Catch2 or GoogleTest (tests only)
- libmpv (Phase 3, clip preview)
- nlohmann/json or similar (config persistence)

---

## Phase 0 — Proof of Concept (Console App)

**Goal:** prove the full pipeline works end-to-end before writing any UI.

**Deliverables**
- Single monitor capture via WGC → `ID3D11Texture2D`
- H.264 encoding via FFmpeg with D3D11 texture input where supported
- In-memory ring buffer storing `AVPacket` for a configurable duration (default 5 min)
- Keyframe index tracking IDR frame positions
- `RegisterHotKey` on `Alt+F10` — on press, find nearest preceding IDR, flush packets to MP4
- Console output only (no tray, no Qt)
- No audio

**What success looks like**
Run the exe, play a game for a few minutes, press `Alt+F10`, get a valid MP4.  
GPU-Z or HWiNFO should show vendor encoder load where hardware encoding is active.

**Estimated scope:** 2–3 weeks

**Key risks at this stage**
- NVENC D3D11 interop setup — registering `ID3D11Texture2D` as NVENC input resource
  requires matching device context; device must be created with `D3D11_CREATE_DEVICE_BGRA_SUPPORT`
- WGC requires a `DispatcherQueue` on the capture thread on some Windows versions
- Keyframe-aligned flush: buffer must not start a clip mid-GOP or the output is undecodable

---

## Phase 1 — Background Service with Basic UI

**Goal:** something you'd actually run every day.

**New features**
- WASAPI loopback audio capture (separate thread)
- Audio AAC encoding via FFmpeg
- Audio/video sync across the ring buffer using DTS timestamps
- Qt system tray icon (start/stop replay buffer, open settings, quit)
- Minimal settings stored to JSON:
  - Output directory
  - Clip duration (1–30 min)
  - Hotkey binding
  - Video quality (CQP level)
  - Monitor selection
- Windows startup via registry or shortcut in `shell:startup`
- `--startreplaybuffer` style autostart flag
- Save notification (Windows toast or Qt balloon)
- DXGI Desktop Duplication fallback (selectable in settings, for DX9 games)

**Estimated scope:** 3–4 weeks on top of Phase 0

**Key risks at this stage**
- Audio/video sync: audio and video ring buffers must be flushed together,
  trimmed to the same wall-clock window. DTS drift is subtle but causes A/V slip.
- WASAPI loopback on some systems has 10–20ms latency that needs compensation.

---

## Phase 2 — Clip Library and Game Awareness

**Goal:** manage your clips, not just save them.

**New features**
- Qt clip library window — thumbnail grid of saved clips, sortable by game/date
- Thumbnail generation via FFmpeg (extract frame at 10% of clip duration)
- Game auto-detection: poll running processes, match against a JSON game list,
  tag saved clips with game name, organize into per-game folders
- Per-game recording settings (override global quality/duration)
- Double-click clip → play in system default player or libmpv preview (see Phase 3)
- Clip rename, delete, open containing folder
- Optional: manual clip mode (start/stop recording, no ring buffer)

**Estimated scope:** 4–6 weeks

---

## Phase 3 — Clip Editor and Compression Export

**Goal:** trim clips and share them without leaving the app.

**New features**
- Embedded libmpv clip preview in the clip library window
- Basic trim editor: set in/out points on a timeline, export trimmed clip
  - Lossless trim via FFmpeg stream copy (instant, no re-encode)
  - Or re-encode with configurable CRF for compression (smaller file)
- Export presets: Original Quality, Compressed (CRF 28), Discord-ready (< 25MB), Twitter/X (< 512MB)
- Progress bar during export
- Configurable output format (MP4, MKV)
- Optional: screenshot from any frame of a clip

**Estimated scope:** 4–6 weeks

---

## Phase 4 — Advanced Capture

**Goal:** match ShadowPlay's actual capture method for maximum compatibility and minimum overhead.

**New features**
- DLL injection game capture (hook `IDXGISwapChain::Present`)
  - Separate 32-bit and 64-bit hook DLLs
  - IPC between hook DLL and host process (shared memory + named event)
  - DX9, DX11, DX12, Vulkan Present variants
  - Anti-cheat considerations: document which games are unsupported
- Capture backend selectable per-game in settings (WGC / DXGI / Hook)
- HDR capture and tone-mapping option (WGC supports HDR on compatible monitors)
- Window capture mode for windowed/borderless games (WGC window picker)

**Estimated scope:** 6–10 weeks (injection alone is substantial)

---

## Phase 5 — Codec and Hardware Expansion

**Goal:** add newer codecs and advanced hardware behavior.

**New features**
- Deeper AMD/Intel tuning and diagnostics beyond the initial FFmpeg backend support
- HEVC (H.265) quality tuning for all backends
- AV1 encoding for RTX 4000+ / RX 7000+ / Intel Arc
- Configurable audio source (specific application audio, not just system-wide)
- Multi-monitor support (record a specific monitor in multi-display setups)

**Estimated scope:** 3–5 weeks

---

## Technical Notes and Gotchas

**Ring buffer must be keyframe-aware**  
You cannot start a clip at an arbitrary packet. The decoder needs an IDR frame first.
The `KeyframeIndex` must track positions of all IDR packets in the buffer.
On save, walk backwards from the save point to find the nearest IDR before the window start.
Set the encoder keyframe interval (`g` in FFmpeg) to a fixed value (e.g. 2 seconds at 60fps = 120 frames)
so the maximum clip-start offset is predictable.

**D3D11 device sharing between capture and encoder**  
Capture and encoder paths must share the same `ID3D11Device` where D3D11 hardware frames are used. Create the device once in `CaptureManager`,
pass it to both `WGCCapture` and `FFmpegVideoEncoder`. Mismatched devices cause texture copy failures.
The device needs `D3D11_CREATE_DEVICE_BGRA_SUPPORT` and should be created on the adapter
that drives the monitor being captured (query via `IDXGIOutput::GetDesc`).

**WGC dispatcher queue requirement**  
`GraphicsCaptureItem` creation and `Direct3D11CaptureFramePool` require a `DispatcherQueue`
on Windows 10 before 22H2. Create one explicitly via `DispatcherQueueController` before
initializing WGC, even if you are not using WinUI.

**NVENC session limits**  
Consumer Nvidia GPUs enforce a maximum of 5 concurrent NVENC sessions (3 on some older cards).
GeForce drivers lifted this restriction for RTX 30 series and later.
For GTX cards, one session for the replay buffer may conflict with a game that uses NVENC
internally (some Unreal Engine titles do). Document this limitation.

**Audio ring buffer alignment**  
Store audio packets with a wall-clock `int64_t pts` in microseconds.
On clip save, compute the video window's start/end PTS, then filter audio packets
to the matching range. AAC frames are ~21ms each — you will have partial frames
at the boundaries; trim the first and last packet's sample data.

**FFmpeg LGPL compliance**  
Use a shared-library FFmpeg build with no GPL components enabled.
Do not include `libx264`, `libx265`, or `fdk-aac` — these are GPL or non-free.
Use non-GPL FFmpeg encoders such as vendor hardware H.264/HEVC paths, Media Foundation fallback, and the built-in `aac` encoder only.
Provide FFmpeg DLLs alongside your installer.
Document that users may replace the DLLs with a GPL build at their own risk.

---

## Open Questions (to revisit each phase)

- Project name — GhostReplay is a working title
- Minimum Windows version — targeting 10 1903 for WGC; consider dropping 10 entirely for 11 in Phase 2+
- Distribution — GitHub Releases with a simple NSIS or WiX installer; no Microsoft Store (signing cost)
- Telemetry — none; opt-in crash reporting via Sentry if the project grows
- Game list for auto-detection — start with a hand-curated JSON, accept PRs

---

*Document version: 0.1 — Phase 0 planning*
