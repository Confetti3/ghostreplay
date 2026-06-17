# Ghost Replay 0.1.0 Beta Release Notes

Ghost Replay 0.1.0 is an early public beta for Windows instant replay capture.

## Highlights

- Background H.264 replay capture with a rolling memory buffer.
- WASAPI loopback audio capture with AAC output.
- Desktop capture, Windows picker window capture, and foreground-game window capture.
- Tray restore/hide behavior for background use.
- Clip library with thumbnails, search, grid/list views, delete confirmation, open/reveal/copy actions, and metadata.
- Clip trim/share export with a clearer Trim -> Preset -> Export flow, progress, cancellation, unique output filenames, and size-targeted presets.
- Refined QML interface with bundled Inter and Space Grotesk typography, accessible shared controls, and a live replay-buffer status strip.
- Startup diagnostics for OS, GPU, FFmpeg, FFprobe, configuration, and output-folder writability.

## Security And Release Notes

- Public packages must use LGPL-compatible FFmpeg builds. Packaging fails if `ffmpeg.exe` reports GPL, libx264, or libx265 flags.
- HEVC capture is disabled in this beta because replay-save muxing is currently validated for H.264.
- Startup registration is opt-in and writes a quoted per-user Windows Run key.
- No telemetry, cloud upload, account system, or network service is included.

## Known Limitations

- Windows-only recording.
- HDR, AV1, game-hook capture, and multi-monitor capture profiles are not included.
- Some fullscreen-exclusive games may require desktop capture or may not capture reliably.
- The package target requires a non-GPL FFmpeg build to complete.
