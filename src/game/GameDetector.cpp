#include "game/GameDetector.h"

#include <Windows.h>
#include <Psapi.h>
#include <dwmapi.h>
#include <algorithm>
#include <cctype>
#include <cwchar>

// ── Static blacklist initialisation ──────────────────────────────

std::vector<std::string> GameDetector::s_blacklist = {
    "explorer.exe",
    "SearchApp.exe",
    "TextInputHost.exe",
    "ApplicationFrameHost.exe",
    "ShellExperienceHost.exe",
    "SystemSettings.exe",
    "Taskmgr.exe",
    "SnippingTool.exe"
};

// ── foregroundProcessName ────────────────────────────────────────

std::string GameDetector::foregroundProcessName()
{
    return processNameForWindow(foregroundWindow());
}

std::string GameDetector::processNameForWindow(HWND hwnd)
{
    if (!hwnd) return {};

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return {};

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return {};

    wchar_t path[MAX_PATH] = {};
    DWORD len = MAX_PATH;
    BOOL ok = QueryFullProcessImageNameW(hProc, 0, path, &len);
    CloseHandle(hProc);
    if (!ok) return {};

    // Extract filename from full path
    const wchar_t* name = wcsrchr(path, L'\\');
    if (name) name++; else name = path;

    // Convert to narrow string
    int req = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
    if (req <= 0) return {};
    std::string result(req - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, name, -1, &result[0], req, nullptr, nullptr);

    // Lowercase for comparison
    std::string lower = result;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return lower;
}

HWND GameDetector::foregroundWindow()
{
    return GetForegroundWindow();
}

std::string GameDetector::windowTitle(HWND hwnd)
{
    if (!hwnd) return {};

    const int length = GetWindowTextLengthW(hwnd);
    if (length <= 0) return {};

    std::wstring title(static_cast<size_t>(length + 1), L'\0');
    const int copied = GetWindowTextW(hwnd, title.data(), length + 1);
    if (copied <= 0) return {};
    title.resize(static_cast<size_t>(copied));

    int req = WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (req <= 0) return {};

    std::string result(static_cast<size_t>(req - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, result.data(), req, nullptr, nullptr);
    return result;
}

bool GameDetector::isProcessBlacklisted(const std::string& process_name)
{
    std::string lower = process_name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    for (const auto& b : s_blacklist)
    {
        if (lower == b)
            return true;
    }
    return false;
}

bool GameDetector::isWindowCapturable(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd))
        return false;

    if (GetAncestor(hwnd, GA_ROOT) != hwnd)
        return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0 || pid == GetCurrentProcessId())
        return false;

    const LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (ex_style & WS_EX_TOOLWINDOW)
        return false;

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked)
        return false;

    RECT rect = {};
    if (!GetWindowRect(hwnd, &rect))
        return false;
    if ((rect.right - rect.left) < 16 || (rect.bottom - rect.top) < 16)
        return false;

    const std::string process_name = processNameForWindow(hwnd);
    return !process_name.empty() && !isProcessBlacklisted(process_name);
}

// ── currentGame ─────────────────────────────────────────────────

std::string GameDetector::currentGame()
{
    HWND hwnd = foregroundWindow();
    if (!isWindowCapturable(hwnd))
        return "Desktop";

    std::string proc = processNameForWindow(hwnd);
    if (proc.empty()) return "Desktop";

    if (isProcessBlacklisted(proc))
        return "Desktop";

    // Strip .exe extension
    if (proc.size() > 4)
    {
        auto ext = proc.substr(proc.size() - 4);
        if (ext == ".exe")
            proc = proc.substr(0, proc.size() - 4);
    }

    // Title-case: capitalise first letter of each word
    if (!proc.empty())
    {
        proc[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(proc[0])));
        for (size_t i = 1; i < proc.size(); ++i)
        {
            if (proc[i - 1] == ' ' || proc[i - 1] == '-' || proc[i - 1] == '_')
                proc[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(proc[i])));
        }
    }

    return proc;
}

// ── setBlacklist / blacklist ─────────────────────────────────────

void GameDetector::setBlacklist(const std::vector<std::string>& blacklist)
{
    s_blacklist = blacklist;
    for (auto& b : s_blacklist)
    {
        std::transform(b.begin(), b.end(), b.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }
}

const std::vector<std::string>& GameDetector::blacklist()
{
    return s_blacklist;
}

// ── sanitizeForFilename ──────────────────────────────────────────

std::string GameDetector::sanitizeForFilename(const std::string& name)
{
    static const char invalid[] = "\\/:*?\"<>|";
    std::string result = name;
    for (char& c : result)
    {
        for (const char* bad = invalid; *bad; ++bad)
        {
            if (c == *bad)
                c = '-';
        }
    }
    // Trim trailing spaces/dots (Windows restrictions)
    while (!result.empty() && (result.back() == ' ' || result.back() == '.'))
        result.pop_back();
    return result;
}
