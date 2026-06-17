#include <catch2/catch_test_macros.hpp>

#ifdef HAS_QT6
#include "util/ReleaseHealth.h"

#include <QString>
#endif

TEST_CASE("release config validation clamps unsupported beta values")
{
#ifndef HAS_QT6
    SKIP("Release health helpers require Qt 6");
#else
    Config config;
    config.capture_source = "bogus";
    config.codec = "hevc";
    config.encoder_backend = "bogus";
    config.cqp = 99;
    config.preset = 99;
    config.fps = 0;
    config.keyframe_interval = 9999;
    config.replay_duration_sec = 1;
    config.audio.channels = 8;
    config.audio.sample_rate = 12345;
    config.audio.bitrate_kbps = 1;
    config.audio.codec = "mp3";
    config.hotkey.modifiers = 0;
    config.hotkey.vk = 999;
    config.output_dir.clear();

    const auto checks = ReleaseHealth::validateConfig(config);

    REQUIRE(config.capture_source == "desktop");
    REQUIRE(config.codec == "h264");
    REQUIRE(config.encoder_backend == "auto");
    REQUIRE(config.cqp == 51);
    REQUIRE(config.preset == 7);
    REQUIRE(config.fps == 1);
    REQUIRE(config.keyframe_interval == 600);
    REQUIRE(config.replay_duration_sec == 15);
    REQUIRE(config.audio.channels == 2);
    REQUIRE(config.audio.sample_rate == 48000);
    REQUIRE(config.audio.bitrate_kbps == 64);
    REQUIRE(config.audio.codec == "aac");
    REQUIRE(config.hotkey.modifiers == 1);
    REQUIRE(config.hotkey.vk == 121);
    REQUIRE(config.output_dir == "clips");
    REQUIRE_FALSE(checks.isEmpty());
#endif
}

TEST_CASE("release config validation accepts supported capture sources")
{
#ifndef HAS_QT6
    SKIP("Release health helpers require Qt 6");
#else
    Config config;
    config.capture_source = "window";
    ReleaseHealth::validateConfig(config);
    REQUIRE(config.capture_source == "window");

    config.capture_source = "game";
    ReleaseHealth::validateConfig(config);
    REQUIRE(config.capture_source == "game");
#endif
}

TEST_CASE("release health checks map cleanly to QML variants")
{
#ifndef HAS_QT6
    SKIP("Release health helpers require Qt 6");
#else
    QList<ReleaseHealth::Check> checks;
    checks.append({ "gpu", "GPU", "success", "Display adapters detected: AMD Radeon. Preferred encoders: AMD AMF." });

    const QVariantList list = ReleaseHealth::toVariantList(checks);
    REQUIRE(list.size() == 1);
    const QVariantMap item = list.front().toMap();
    REQUIRE(item.value("id").toString() == "gpu");
    REQUIRE(item.value("severity").toString() == "success");
    REQUIRE(item.value("message").toString().contains("AMD AMF"));
#endif
}

TEST_CASE("diagnostics include release and capture status")
{
#ifndef HAS_QT6
    SKIP("Release health helpers require Qt 6");
#else
    Config config;
    QList<ReleaseHealth::Check> checks;
    checks.append({ "config", "Configuration", "success", "Settings are valid." });

    const QString text = ReleaseHealth::diagnosticsText(config, checks, false, "Capture unavailable");

    REQUIRE(text.contains("Ghost Replay diagnostics"));
    REQUIRE(text.contains("Version:"));
    REQUIRE(text.contains("Capture available: no"));
    REQUIRE(text.contains("Capture unavailable"));
    REQUIRE(text.contains("Capture source: desktop"));
    REQUIRE(text.contains("Configuration"));
#endif
}
