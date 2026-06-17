# Release Checklist

Use this checklist before publishing a Ghost Replay build.

## Build Inputs

- Build commands run from a Visual Studio Developer PowerShell or x64 Native Tools Command Prompt.
- `QT6_DIR` points to the intended Qt 6 MSVC runtime.
- `FFMPEG_DIR` points to a shared LGPL-compatible FFmpeg dev build.
- `config/ghostreplay.example.json` contains public defaults only.
- No local clips, logs, screenshots, test binaries, or debug archives are staged.
- The release output directory is clean before packaging so stale FFmpeg DLL versions cannot be included.

## Validation

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON `
  -DQT6_DIR="C:\Qt\6.x.x\msvc2022_64" `
  -DFFMPEG_DIR="C:\ffmpeg-lgpl"

cmake --build build --config Release --target ghostreplay_tests
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target ghostreplay
cmake --build build --config Release --target package_portable
```

## Manual Smoke

- Launch `ghostreplay.exe`; the main window appears and can hide/restore from the tray.
- Overview shows either running capture or a clear, actionable unavailable state.
- Settings save successfully and invalid values are clamped by health checks.
- Save replay handles an empty buffer with a visible notification.
- Clips page refreshes, filters, opens, reveals, copies, and confirms deletion.
- Share export creates a new numbered filename instead of overwriting an existing export.
- Diagnostics copy to the clipboard and do not include secrets.

## Package Audit

- `README_EARLY_ACCESS.md`, `RELEASE_NOTES.md`, `THIRD_PARTY_NOTICES.md`, `LICENSE`, `SECURITY.md`, and `ghostreplay.json` are present in the package.
- `ghostreplay.log`, `ghostreplay_tests.exe`, `AGENTS.md`, and `clips/` are absent from the package.
- `ffmpeg.exe -hide_banner -buildconf` does not include `--enable-gpl`, `--enable-libx264`, or `--enable-libx265`.
- Exactly one runtime DLL for each FFmpeg component is present: `avcodec`, `avformat`, `avutil`, `swresample`, and `swscale`.
