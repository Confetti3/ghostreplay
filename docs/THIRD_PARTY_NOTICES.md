# Third-Party Notices

Ghost Replay depends on these third-party components in the portable beta:

- Qt 6 runtime libraries. See Qt's license terms for the exact modules shipped with your package.
- FFmpeg shared libraries and command-line tools. Public packages must use LGPL-compatible builds without GPL/non-free encoders.
- nlohmann/json 3.11.3, used as a header-only JSON library.
- Catch2 3.6.0, used for tests only and not required at runtime.
- Inter variable font, bundled for UI text under the SIL Open Font License 1.1.
- Space Grotesk variable font, bundled for headings and status labels under the SIL Open Font License 1.1.

The portable package includes runtime files required to launch Ghost Replay. Users may replace FFmpeg binaries with compatible builds at their own risk.

Ghost Replay source code is distributed under the MIT License. Third-party components remain under their respective licenses.
