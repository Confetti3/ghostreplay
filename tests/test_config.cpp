#include <catch2/catch_test_macros.hpp>

#include "util/Config.h"

#ifdef HAS_QT6
#include "util/ReleaseHealth.h"
#endif

#include <filesystem>
#include <fstream>

namespace
{
std::filesystem::path tempConfigPath()
{
    return std::filesystem::temp_directory_path() / "ghostreplay-config-test.json";
}
}

TEST_CASE("config load honors JSON booleans")
{
    const auto path = tempConfigPath();
    std::filesystem::remove(path);

    {
        std::ofstream out(path);
        out << R"({
  "launch_at_startup": true,
  "audio": {
    "enabled": false
  }
})";
    }

    Config config;
    REQUIRE(config.load(path.string()));
    REQUIRE(config.launch_at_startup);
    REQUIRE_FALSE(config.audio.enabled);

    std::filesystem::remove(path);
}

TEST_CASE("config command line parser ignores invalid integers and accepts flags")
{
    Config config;
    char app[] = "ghostreplay";
    char badFps[] = "--fps=not-a-number";
    char duration[] = "--duration=120";
    char audioOff[] = "--audio-off";
    char* argv[] = { app, badFps, duration, audioOff };

    REQUIRE_NOTHROW(config.parseArgs(4, argv));
    REQUIRE(config.fps == 60);
    REQUIRE(config.replay_duration_sec == 120);
    REQUIRE_FALSE(config.audio.enabled);
}

TEST_CASE("release example config uses public safe defaults")
{
#ifndef GHOST_REPLAY_TEST_SOURCE_DIR
    SKIP("Source directory is not available");
#else
    const auto path = std::filesystem::path(GHOST_REPLAY_TEST_SOURCE_DIR)
        / "config"
        / "ghostreplay.example.json";

    Config config;
    REQUIRE(config.load(path.string()));
    REQUIRE(config.codec == "h264");
    REQUIRE(config.capture_source == "desktop");
    REQUIRE(config.encoder_backend == "auto");
    REQUIRE_FALSE(config.launch_at_startup);
    REQUIRE(config.output_dir == "clips");
    REQUIRE(config.log_level >= 1);

#ifdef HAS_QT6
    const auto checks = ReleaseHealth::validateConfig(config);
    REQUIRE(config.codec == "h264");
    REQUIRE(config.capture_source == "desktop");
    REQUIRE(config.audio.codec == "aac");
    REQUIRE(checks.size() == 1);
    REQUIRE(checks.front().severity == "success");
#endif
#endif
}
