# Contributing to Ghost Replay

Thank you for your interest in contributing to Ghost Replay! As an open-source, local-first capture client, we appreciate community fixes and features.

## Development Setup

To build and test the project locally, please follow these steps:

### Prerequisites
- **Operating System**: Windows 10 (1903 or newer) or Windows 11.
- **Compiler**: Visual Studio 2022 with MSVC C++20 compiler tools.
- **Build Tool**: CMake 3.25 or newer.
- **Libraries**:
  - Qt 6 (Core, Widgets, Quick, QuickControls2, QuickDialogs2, Multimedia, Network).
  - FFmpeg shared development libraries (must be LGPL-compatible for packaging).

### Building from Source

Run the configuration and build from a **Visual Studio Developer PowerShell** or **x64 Native Tools Command Prompt**:

```powershell
# Configure the project
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON `
  -DQT6_DIR="C:\Qt\6.x.x\msvc2022_64" `
  -DFFMPEG_DIR="C:\ffmpeg"

# Compile Release binary
cmake --build build --config Release
```

### Running Tests

We run unit tests to verify configuration parser, encoder selector logic, and file helpers:

```powershell
# Build the test executable
cmake --build build --config Release --target ghostreplay_tests

# Execute test suite
.\build\Release\ghostreplay_tests.exe
```

## Pull Request Guidelines

1. **Create a Topic Branch**: Build your changes on a descriptive branch name (e.g. `fix/hotkey-conflict`).
2. **Write Unit Tests**: If you are adding configuration options or utility logic, please add matching assertions in `tests/`.
3. **Verify Compliance**:
   - Ensure the app builds from a clean directory.
   - Run the unit tests and make sure they all pass.
   - Run `cmake --build build --config Release --target package_portable` to check that the LGPL audit passes.
4. **Clean Code**: Keep formatting clean and preserve comments/licensing headers.

## Security Disclosures

If you discover a security vulnerability, please do **not** open a public issue. Review our [SECURITY.md](SECURITY.md) to report it privately.
