#include "util/Config.h"
#include "util/Logger.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <direct.h>
#include <sys/stat.h>
#include <Windows.h>

using json = nlohmann::json;

// ── Helpers ─────────────────────────────────────────────────────

namespace {

int opt_int(const json& j, const char* key, int dflt)
{
    if (!j.contains(key)) return dflt;
    auto& v = j[key];
    return v.is_number_integer() ? v.get<int>() : dflt;
}

bool opt_bool(const json& j, const char* key, bool dflt)
{
    if (!j.contains(key)) return dflt;

    const auto& v = j[key];
    if (v.is_boolean())
        return v.get<bool>();
    if (v.is_number_integer())
        return v.get<int>() != 0;
    if (v.is_string())
    {
        std::string value = v.get<std::string>();
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (value == "true" || value == "1" || value == "yes" || value == "on")
            return true;
        if (value == "false" || value == "0" || value == "no" || value == "off")
            return false;
    }

    return dflt;
}

std::string opt_str(const json& j, const char* key, const std::string& dflt)
{
    if (!j.contains(key)) return dflt;
    auto& v = j[key];
    return v.is_string() ? v.get<std::string>() : dflt;
}

bool parse_int_arg(const std::string& value, int& out)
{
    try
    {
        size_t consumed = 0;
        const int parsed = std::stoi(value, &consumed, 0);
        if (consumed != value.size())
            return false;
        out = parsed;
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

} // namespace

// ── Load ────────────────────────────────────────────────────────

bool Config::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_WARNING("Config: could not open " + path + ", using defaults");
        return false;
    }

    try
    {
        json j = json::parse(file);

        // Flat keys (backward-compatible with old .conf names)
        monitor_index      = opt_int(j, "monitor_index", 0);
        capture_width      = opt_int(j, "capture_width", 0);
        capture_height     = opt_int(j, "capture_height", 0);
        capture_source     = opt_str(j, "capture_source", "desktop");
        replay_duration_sec = opt_int(j, "replay_duration_sec", 300);
        fps                = opt_int(j, "fps", 60);

        codec              = opt_str(j, "codec", "h264");
        encoder_backend    = opt_str(j, "encoder_backend", "auto");
        cqp                = opt_int(j, "cqp", 23);
        keyframe_interval  = opt_int(j, "keyframe_interval", 120);
        preset             = opt_int(j, "preset", 4);

        hotkey.modifiers   = static_cast<unsigned int>(opt_int(j, "hotkey_modifiers", 1));
        hotkey.vk          = static_cast<unsigned int>(opt_int(j, "hotkey_vk", 121));

        output_dir         = opt_str(j, "output_dir", "clips");
        launch_at_startup  = opt_bool(j, "launch_at_startup", false);
        log_file           = opt_str(j, "log_file", "ghostreplay.log");
        log_level          = opt_int(j, "log_level", 0);

        // Audio sub-object
        if (j.contains("audio") && j["audio"].is_object())
        {
            auto& a = j["audio"];
            audio.enabled     = opt_bool(a, "enabled", true);
            audio.device_id   = opt_str(a, "device_id", "");
            audio.channels    = opt_int(a, "channels", 2);
            audio.sample_rate = opt_int(a, "sample_rate", 48000);
            audio.bitrate_kbps = opt_int(a, "bitrate_kbps", 192);
            audio.codec       = opt_str(a, "codec", "aac");
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Config: JSON parse error in " + path + " — " + e.what());
        return false;
    }

    LOG_INFO("Config: loaded from " + path);
    return true;
}

// ── Save ────────────────────────────────────────────────────────

bool Config::save(const std::string& path) const
{
    json j;

    j["monitor_index"]        = monitor_index;
    j["capture_width"]        = capture_width;
    j["capture_height"]       = capture_height;
    j["capture_source"]       = capture_source;
    j["replay_duration_sec"]  = replay_duration_sec;
    j["fps"]                  = fps;

    j["codec"]                = codec;
    j["encoder_backend"]      = encoder_backend;
    j["cqp"]                  = cqp;
    j["keyframe_interval"]    = keyframe_interval;
    j["preset"]               = preset;

    j["hotkey_modifiers"]     = hotkey.modifiers;
    j["hotkey_vk"]            = hotkey.vk;

    j["output_dir"]           = output_dir;
    j["launch_at_startup"]    = launch_at_startup;
    j["log_file"]             = log_file;
    j["log_level"]            = log_level;

    json a;
    a["enabled"]      = audio.enabled;
    a["device_id"]    = audio.device_id;
    a["channels"]     = audio.channels;
    a["sample_rate"]  = audio.sample_rate;
    a["bitrate_kbps"] = audio.bitrate_kbps;
    a["codec"]        = audio.codec;
    j["audio"]        = a;

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(2) << "\n";
    return true;
}

// ── Command-line args ───────────────────────────────────────────

void Config::parseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);

        if (arg == "--audio-off")
        {
            audio.enabled = false;
            continue;
        }
        if (arg == "--audio-on")
        {
            audio.enabled = true;
            continue;
        }
        if (arg == "--launch-at-startup")
        {
            launch_at_startup = true;
            continue;
        }
        if (arg == "--no-launch-at-startup")
        {
            launch_at_startup = false;
            continue;
        }
        if (arg == "--minimized" || arg == "--tray")
        {
            start_minimized = true;
            continue;
        }

        auto eq = arg.find('=');
        if (eq == std::string::npos) continue;

        std::string key = arg.substr(0, eq);
        std::string val = arg.substr(eq + 1);

        int parsed = 0;
        auto assignInt = [&](int& target, const char* label)
        {
            if (parse_int_arg(val, parsed))
                target = parsed;
            else
                LOG_WARNING(std::string("Config: ignoring invalid value for ") + label + ": " + val);
        };

        if (key == "--monitor")              assignInt(monitor_index, "--monitor");
        else if (key == "--duration")        assignInt(replay_duration_sec, "--duration");
        else if (key == "--cqp")             assignInt(cqp, "--cqp");
        else if (key == "--fps")             assignInt(fps, "--fps");
        else if (key == "--capture-source")  capture_source      = val;
        else if (key == "--encoder-backend") encoder_backend     = val;
        else if (key == "--keyframe-interval") assignInt(keyframe_interval, "--keyframe-interval");
        else if (key == "--output-dir")      output_dir          = val;
        else if (key == "--log-file")        log_file            = val;
        else if (key == "--log-level")       assignInt(log_level, "--log-level");
    }
}

// ── Windows Startup Registry Toggle ─────────────────────────────

static const wchar_t* RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* VALUE_NAME = L"GhostReplay";

bool Config::setStartupEnabled(bool enable)
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0,
                                KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        LOG_ERROR("Config::setStartup: RegOpenKeyEx failed (" + std::to_string(result) + ")");
        return false;
    }

    bool ok = false;
    if (enable)
    {
        wchar_t exe_path[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            std::wstring command = L"\"";
            command.append(exe_path, exe_path + len);
            command.append(L"\" --minimized");
            result = RegSetValueExW(hKey, VALUE_NAME, 0, REG_SZ,
                                    reinterpret_cast<const BYTE*>(command.c_str()),
                                    static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
            ok = (result == ERROR_SUCCESS);
            if (!ok)
                LOG_ERROR("Config::setStartup: RegSetValueEx failed (" + std::to_string(result) + ")");
        }
        else
        {
            LOG_ERROR("Config::setStartup: GetModuleFileName failed");
        }
    }
    else
    {
        result = RegDeleteValueW(hKey, VALUE_NAME);
        ok = (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
        if (!ok)
            LOG_WARNING("Config::setStartup: RegDeleteValue failed (" + std::to_string(result) + ")");
    }

    RegCloseKey(hKey);
    return ok;
}

bool Config::isStartupEnabled()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return false;

    DWORD type;
    result = RegQueryValueExW(hKey, VALUE_NAME, nullptr, &type, nullptr, nullptr);
    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}
