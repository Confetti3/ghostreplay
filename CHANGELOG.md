# Changelog

All notable changes to the Ghost Replay project will be documented in this file. This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-06-26

This is the first public preview/beta release of Ghost Replay, providing an efficient, low-overhead local replay buffer capture tool for Windows.

### Added
- **Rolling Replay Buffer**: Background video/audio recording held in memory/disk-backed ring buffer until hotkey triggers.
- **Hardware-Accelerated Video Encoding**: Integrated Nvidia NVENC, AMD AMF, Intel Quick Sync (QSV), and Windows Media Foundation (WMF) H.264 encoders.
- **Loopback Audio Capture**: WASAPI-based system audio loopback capture mapped to AAC audio.
- **Dynamic UI**: Refined Qt/QML client interface utilizing Inter and Space Grotesk typography, featuring a real-time status monitor and glassmorphic panels.
- **Clip Management**: Integrated browser supporting list/grid views, metadata previews, list-hover tooltips, and deletion confirmations.
- **Trim & Share Export**: In-app video trimming with target-size export presets (Discord, Email, etc.), progress indicators, and cancel options.
- **Diagnostics Dashboard**: Automatic start-up validation suite covering Windows OS compatibility, encoder accessibility, FFmpeg status, and directory permissions.
- **Single-Instance IPC**: Socket-based application listener preventing multiple concurrent capture instances, automatically focusing the active window on collision.
- **System Tray Integration**: Background running support allowing user to minimize to tray and control recording from the taskbar context menu.
