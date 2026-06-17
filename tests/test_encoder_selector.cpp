#include <catch2/catch_test_macros.hpp>

#include "encode/EncoderSelector.h"

#include <set>

TEST_CASE("encoder selector prefers the monitor vendor backend")
{
    const auto candidates = buildEncoderCandidates(
        EncoderBackend::Auto,
        GpuVendor::AMD,
        EncoderConfig::Codec::H264);

    REQUIRE(candidates.size() >= 2);
    REQUIRE(candidates.front().backend == EncoderBackend::AMF);
    REQUIRE(candidates.front().encoder_name == "h264_amf");
}

TEST_CASE("encoder selector retries h264 before media foundation when hevc is preferred")
{
    const auto candidates = buildEncoderCandidates(
        EncoderBackend::Auto,
        GpuVendor::Nvidia,
        EncoderConfig::Codec::HEVC);

    REQUIRE(candidates.size() >= 3);
    REQUIRE(candidates[0].encoder_name == "hevc_nvenc");
    REQUIRE(candidates[1].encoder_name == "h264_nvenc");
    REQUIRE(candidates.back().encoder_name == "h264_mf");
    REQUIRE(candidates.back().compatibility_fallback);
}

TEST_CASE("encoder selector honors manual backend before fallback")
{
    const auto candidates = buildEncoderCandidates(
        EncoderBackend::QSV,
        GpuVendor::Nvidia,
        EncoderConfig::Codec::H264);

    REQUIRE(candidates.size() == 2);
    REQUIRE(candidates[0].encoder_name == "h264_qsv");
    REQUIRE(candidates[1].encoder_name == "h264_mf");
}

TEST_CASE("encoder selector filters unavailable ffmpeg encoders")
{
    const std::set<std::string> available{ "h264_amf", "h264_mf" };
    const auto candidates = buildEncoderCandidates(
        EncoderBackend::Auto,
        GpuVendor::AMD,
        EncoderConfig::Codec::HEVC,
        available);

    REQUIRE(candidates.size() == 2);
    REQUIRE(candidates[0].encoder_name == "h264_amf");
    REQUIRE(candidates[1].encoder_name == "h264_mf");
}
