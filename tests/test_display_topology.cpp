#include <catch2/catch_test_macros.hpp>

#ifdef HAS_QT6
#include "ui/DisplayTopologyMonitor.h"
#endif

TEST_CASE("display comparison ignores identical snapshots")
{
#ifndef HAS_QT6
    SKIP("Display topology helpers require Qt 6");
#else
    DisplaySnapshot before;
    before.valid = true;
    before.deviceName = "DISPLAY1";
    before.label = "DISPLAY1";
    before.pixelBounds = QRect(0, 0, 1920, 1080);
    before.dpiX = 96;
    before.dpiY = 96;
    before.scale = 1.0;

    const DisplayChangeDecision decision =
        DisplayTopologyMonitor::compareSnapshots(before, before, true);

    REQUIRE_FALSE(decision.changed);
    REQUIRE_FALSE(decision.shouldRestartCapture);
#endif
}

TEST_CASE("native auto capture restarts when display resolution changes")
{
#ifndef HAS_QT6
    SKIP("Display topology helpers require Qt 6");
#else
    DisplaySnapshot before;
    before.valid = true;
    before.deviceName = "DISPLAY1";
    before.pixelBounds = QRect(0, 0, 1920, 1080);
    before.dpiX = 96;
    before.dpiY = 96;
    before.scale = 1.0;

    DisplaySnapshot after = before;
    after.pixelBounds = QRect(0, 0, 2560, 1440);

    const DisplayChangeDecision decision =
        DisplayTopologyMonitor::compareSnapshots(before, after, true);

    REQUIRE(decision.changed);
    REQUIRE(decision.resolutionChanged);
    REQUIRE(decision.shouldRestartCapture);
#endif
}

TEST_CASE("native auto capture restarts when display DPI changes")
{
#ifndef HAS_QT6
    SKIP("Display topology helpers require Qt 6");
#else
    DisplaySnapshot before;
    before.valid = true;
    before.deviceName = "DISPLAY1";
    before.pixelBounds = QRect(0, 0, 1920, 1080);
    before.dpiX = 96;
    before.dpiY = 96;
    before.scale = 1.0;

    DisplaySnapshot after = before;
    after.dpiX = 144;
    after.dpiY = 144;
    after.scale = 1.5;

    const DisplayChangeDecision decision =
        DisplayTopologyMonitor::compareSnapshots(before, after, true);

    REQUIRE(decision.changed);
    REQUIRE(decision.dpiChanged);
    REQUIRE(decision.shouldRestartCapture);
#endif
}

TEST_CASE("manual capture resolution preserves configured output size")
{
#ifndef HAS_QT6
    SKIP("Display topology helpers require Qt 6");
#else
    DisplaySnapshot before;
    before.valid = true;
    before.deviceName = "DISPLAY1";
    before.pixelBounds = QRect(0, 0, 1920, 1080);
    before.dpiX = 96;
    before.dpiY = 96;
    before.scale = 1.0;

    DisplaySnapshot after = before;
    after.pixelBounds = QRect(0, 0, 2560, 1440);
    after.dpiX = 144;
    after.dpiY = 144;
    after.scale = 1.5;

    const DisplayChangeDecision decision =
        DisplayTopologyMonitor::compareSnapshots(before, after, false);

    REQUIRE(decision.changed);
    REQUIRE(decision.resolutionChanged);
    REQUIRE(decision.dpiChanged);
    REQUIRE_FALSE(decision.shouldRestartCapture);
#endif
}

TEST_CASE("missing selected monitor falls back and requests restart")
{
#ifndef HAS_QT6
    SKIP("Display topology helpers require Qt 6");
#else
    DisplaySnapshot before;
    before.valid = true;
    before.configuredMonitorIndex = 1;
    before.resolvedMonitorIndex = 1;
    before.deviceName = "DISPLAY2";
    before.pixelBounds = QRect(1920, 0, 1920, 1080);
    before.dpiX = 96;
    before.dpiY = 96;
    before.scale = 1.0;

    DisplaySnapshot after;
    after.valid = true;
    after.selectedMonitorMissing = true;
    after.configuredMonitorIndex = 1;
    after.resolvedMonitorIndex = 0;
    after.deviceName = "DISPLAY1";
    after.pixelBounds = QRect(0, 0, 1920, 1080);
    after.dpiX = 96;
    after.dpiY = 96;
    after.scale = 1.0;

    const DisplayChangeDecision decision =
        DisplayTopologyMonitor::compareSnapshots(before, after, false);

    REQUIRE(decision.changed);
    REQUIRE(decision.monitorChanged);
    REQUIRE(decision.selectedMonitorMissing);
    REQUIRE(decision.shouldRestartCapture);
#endif
}
