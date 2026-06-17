# Ghost Replay Early Access

Ghost Replay is a Windows instant-replay recorder for saving recent desktop/gameplay moments to MP4.

## Requirements

- Windows 10 or newer.
- Supported H.264 video encoder: Nvidia NVENC, AMD AMF, Intel Quick Sync, or Windows Media Foundation compatibility fallback.
- Recent GPU/display driver.
- Writable clips folder.

Auto encoder selection is the default. If recording fails, use Settings or diagnostics to force NVENC, AMF, Quick Sync, or Windows compatibility mode.

## Quick Start

1. Run `ghostreplay.exe`.
2. Leave capture running while you play or work.
3. Press `Alt+F10` or click `Save replay`.
4. Open, copy, trim, or share saved clips from the Clips page.

Closing the window hides Ghost Replay to the tray. Use the tray action `Show Ghost Replay` to bring it back. Use `Exit` from the tray to fully quit.

## Troubleshooting

- If capture is unavailable, open Overview and use `Retry`, `Log`, or `Diagnostics`.
- If `ffmpeg.exe` is missing, recording may still run, but share/export presets may be unavailable.
- If `ffprobe.exe` is missing, clip metadata, thumbnails, and previews may be limited.
- If the output folder is not writable, choose another folder in Settings.
- If the hotkey does not work, another app may own it. Change the shortcut in Settings and restart Ghost Replay.
- If a share export already exists, Ghost Replay creates a numbered filename instead of overwriting it.

## Files

- Default clips folder: `clips`
- Settings file: `ghostreplay.json`
- Log file: `ghostreplay.log`
- Release notes: `RELEASE_NOTES.md`
- License: `LICENSE`

## Known Limitations

- Recording is Windows-only in this beta.
- Desktop capture is the safest capture source. Window capture uses the Windows picker, and Game captures the current foreground game window without hooks.
- HEVC capture, AV1, game-hook capture, and HDR capture are not included yet.
- Windows compatibility mode may use more CPU/GPU time than vendor hardware encoders.
- Some fullscreen-exclusive games may not capture reliably with the current desktop capture backend.
