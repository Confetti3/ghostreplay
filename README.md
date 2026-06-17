# Ghost Replay

Ghost Replay is a Windows instant-replay recorder for saving recent desktop, window, or foreground-game moments to MP4.

## Status

This repository is in public beta shape. The release build uses H.264 capture, WASAPI loopback audio, a rolling replay buffer, a Qt/QML desktop UI, tray controls, clip browsing, trimming, and FFmpeg-based share exports.

## Requirements

- Windows 10 1903 or newer.
- Visual Studio 2022 with MSVC C++20 tools.
- CMake 3.25 or newer.
- Qt 6 with Core, Widgets, Quick, QuickControls2, QuickDialogs2, and Multimedia.
- A shared, LGPL-compatible FFmpeg dev build with `include`, `lib`, and runtime DLLs. Do not ship GPL-enabled FFmpeg builds for public releases.
- Run CMake builds from a Visual Studio Developer PowerShell or x64 Native Tools Command Prompt so deployment tools can find the MSVC runtime.

## Security And Privacy

Ghost Replay records and stores data locally. It does not include telemetry,
cloud upload, account features, or analytics. See [SECURITY.md](SECURITY.md) for
the vulnerability reporting process and release packaging expectations.

## Configure And Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON `
  -DQT6_DIR="C:\Qt\6.x.x\msvc2022_64" `
  -DFFMPEG_DIR="C:\ffmpeg"

cmake --build build --config Release
```

If `nlohmann_json` or Catch2 are already installed with CMake package files,
Ghost Replay uses those packages. Otherwise CMake fetches the pinned versions
listed in [docs/THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md).

Build tests with:

```powershell
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Release --target ghostreplay_tests
ctest --test-dir build -C Release --output-on-failure
```

## Run

```powershell
build\Release\ghostreplay.exe
```

The app writes settings to `ghostreplay.json`, logs to `ghostreplay.log`, and clips to `clips` by default. Closing the window hides the app to the tray; use the tray menu to show it again or exit.

## Package

```powershell
cmake --build build --config Release --target package_portable
```

The package step stages `dist\GhostReplay-<version>-win64-portable` and checks packaged `ffmpeg.exe` for GPL/libx264/libx265 flags before completing.
Release packages use [config/ghostreplay.example.json](config/ghostreplay.example.json) for public defaults, not your local `ghostreplay.json`.

See [docs/RELEASE_CHECKLIST.md](docs/RELEASE_CHECKLIST.md) before publishing.

## Troubleshooting

- Use Overview > Diagnostics when capture is unavailable.
- Use Settings > Output to choose a writable clips folder.
- If the save hotkey is already taken, change it in Settings and save.
- Window capture opens the Windows picker when you retry capture.
- HEVC capture is intentionally disabled in this beta until replay-save muxing is validated end to end.

## UI Assets

The Qt/QML interface bundles Inter and Space Grotesk from Google Fonts for a
consistent release appearance. Both fonts are distributed under the SIL Open
Font License; see [docs/THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md)
and `resources/fonts`.
