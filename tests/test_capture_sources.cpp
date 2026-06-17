#include <catch2/catch_test_macros.hpp>

#include "capture/CaptureTarget.h"
#include "game/GameDetector.h"

TEST_CASE("capture source parser accepts desktop window and game")
{
    REQUIRE(captureSourceFromString("desktop") == CaptureSource::Desktop);
    REQUIRE(captureSourceFromString("window") == CaptureSource::Window);
    REQUIRE(captureSourceFromString("game") == CaptureSource::Game);
    REQUIRE(captureSourceFromString("WINDOW") == CaptureSource::Window);
    REQUIRE(captureSourceFromString("bogus") == CaptureSource::Desktop);
}

TEST_CASE("game detector blacklist comparison is normalized")
{
    GameDetector::setBlacklist({ "Explorer.exe", "GhostReplay.exe" });

    REQUIRE(GameDetector::isProcessBlacklisted("explorer.exe"));
    REQUIRE(GameDetector::isProcessBlacklisted("GHOSTREPLAY.EXE"));
    REQUIRE_FALSE(GameDetector::isProcessBlacklisted("game.exe"));
}

TEST_CASE("game detector sanitizes clip names for Windows paths")
{
    REQUIRE(GameDetector::sanitizeForFilename("Game:Mode/Win.") == "Game-Mode-Win");
}
