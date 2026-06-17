#pragma once

#include "encode/EncoderConfig.h"

#include <set>
#include <string>
#include <vector>

enum class EncoderBackend
{
    Auto,
    NVENC,
    AMF,
    QSV,
    MF
};

enum class GpuVendor
{
    Unknown,
    Nvidia,
    AMD,
    Intel
};

struct EncoderCandidate
{
    EncoderBackend backend = EncoderBackend::Auto;
    EncoderConfig::Codec codec = EncoderConfig::Codec::H264;
    std::string encoder_name;
    std::string display_name;
    bool compatibility_fallback = false;
};

EncoderBackend encoderBackendFromString(const std::string& value);
EncoderBackend encoderBackendFromConfig(EncoderConfig::Backend backend);
EncoderConfig::Backend encoderConfigBackendFromString(const std::string& value);
std::string encoderBackendToString(EncoderBackend backend);
std::string encoderBackendDisplayName(EncoderBackend backend);
std::string codecToString(EncoderConfig::Codec codec);
std::string ffmpegEncoderName(EncoderBackend backend, EncoderConfig::Codec codec);
EncoderBackend preferredBackendForVendor(GpuVendor vendor);

std::vector<EncoderCandidate> buildEncoderCandidates(
    EncoderBackend requested_backend,
    GpuVendor vendor,
    EncoderConfig::Codec preferred_codec,
    const std::set<std::string>& available_encoders = {});
