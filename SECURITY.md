# Security Policy

## Supported Releases

Ghost Replay is currently in public beta. Security fixes are prioritized for the
latest published beta build and the `main` development branch.

## Privacy Model

Ghost Replay is a local desktop recorder. It does not include telemetry,
analytics, cloud upload, or account features. Clips, logs, and configuration are
stored on the user's machine unless the user moves or shares them.

Release packages must use an LGPL-compatible FFmpeg build. GPL/libx264/libx265
enabled FFmpeg binaries are intentionally blocked by the package target.

## Reporting A Vulnerability

Report security issues through the project's private maintainer channel or a
GitHub Security Advisory before posting technical details publicly. Include the
affected version, reproduction steps, expected impact, and any relevant logs with
personal data removed.

Please do not attach private clips, tokens, account data, or unrelated system
logs when reporting an issue.
