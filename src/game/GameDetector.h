#pragma once

#include <Windows.h>

#include <string>
#include <vector>

// ── Game detection utility ───────────────────────────────────────
// Polls the foreground window to determine what game or app is
// currently active, so clips can be tagged automatically.

class GameDetector
{
public:
    // Return the current foreground process executable name
    // (e.g. "cs2.exe"). Returns empty string on failure.
    static std::string foregroundProcessName();
    static std::string processNameForWindow(HWND hwnd);
    static HWND foregroundWindow();
    static std::string windowTitle(HWND hwnd);
    static bool isWindowCapturable(HWND hwnd);
    static bool isProcessBlacklisted(const std::string& process_name);

    // Return a user-friendly game name (exe name without .exe,
    // title-cased). Falls back to "Desktop" when no game
    // foreground or process is blacklisted.
    static std::string currentGame();

    // Set blacklist — process names to ignore (return "Desktop")
    static void setBlacklist(const std::vector<std::string>& blacklist);
    static const std::vector<std::string>& blacklist();

    // Sanitize a game name for use in filenames
    // (removes characters invalid on Windows: \/:*?"<>|)
    static std::string sanitizeForFilename(const std::string& name);

private:
    static std::vector<std::string> s_blacklist;
};
