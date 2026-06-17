#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

enum class CaptureSource
{
    Desktop,
    Window,
    Game
};

inline std::string normalizedCaptureSource(std::string_view value)
{
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (normalized == "window" || normalized == "game")
        return normalized;
    return "desktop";
}

inline CaptureSource captureSourceFromString(std::string_view value)
{
    const std::string normalized = normalizedCaptureSource(value);
    if (normalized == "window")
        return CaptureSource::Window;
    if (normalized == "game")
        return CaptureSource::Game;
    return CaptureSource::Desktop;
}

inline const char* captureSourceToString(CaptureSource source)
{
    switch (source)
    {
    case CaptureSource::Window: return "window";
    case CaptureSource::Game: return "game";
    case CaptureSource::Desktop:
    default: return "desktop";
    }
}
