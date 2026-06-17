#include "encode/EncoderSelector.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace
{
std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool hasEncoder(const std::set<std::string>& available, const std::string& name)
{
    return available.empty() || available.contains(name);
}

void appendCandidate(std::vector<EncoderCandidate>& candidates,
                     EncoderBackend backend,
                     EncoderConfig::Codec codec,
                     bool compatibility_fallback,
                     const std::set<std::string>& available)
{
    const std::string encoder_name = ffmpegEncoderName(backend, codec);
    if (encoder_name.empty() || !hasEncoder(available, encoder_name))
        return;

    EncoderCandidate candidate;
    candidate.backend = backend;
    candidate.codec = codec;
    candidate.encoder_name = encoder_name;
    candidate.display_name = encoderBackendDisplayName(backend) + " " + codecToString(codec);
    candidate.compatibility_fallback = compatibility_fallback;
    candidates.push_back(std::move(candidate));
}

void appendBackendCandidates(std::vector<EncoderCandidate>& candidates,
                             EncoderBackend backend,
                             EncoderConfig::Codec preferred_codec,
                             const std::set<std::string>& available)
{
    if (backend == EncoderBackend::Auto)
        return;

    if (backend == EncoderBackend::MF)
    {
        appendCandidate(candidates, backend, EncoderConfig::Codec::H264, true, available);
        return;
    }

    appendCandidate(candidates, backend, preferred_codec, false, available);
    if (preferred_codec == EncoderConfig::Codec::HEVC)
        appendCandidate(candidates, backend, EncoderConfig::Codec::H264, false, available);
}

void appendUniqueBackend(std::vector<EncoderBackend>& backends, EncoderBackend backend)
{
    if (backend == EncoderBackend::Auto)
        return;
    if (std::find(backends.begin(), backends.end(), backend) == backends.end())
        backends.push_back(backend);
}
}

EncoderBackend encoderBackendFromString(const std::string& value)
{
    const std::string normalized = lower(value);
    if (normalized == "nvenc" || normalized == "nvidia")
        return EncoderBackend::NVENC;
    if (normalized == "amf" || normalized == "amd")
        return EncoderBackend::AMF;
    if (normalized == "qsv" || normalized == "quick_sync" || normalized == "intel")
        return EncoderBackend::QSV;
    if (normalized == "mf" || normalized == "mediafoundation" || normalized == "media_foundation")
        return EncoderBackend::MF;
    return EncoderBackend::Auto;
}

EncoderBackend encoderBackendFromConfig(EncoderConfig::Backend backend)
{
    switch (backend)
    {
    case EncoderConfig::Backend::NVENC: return EncoderBackend::NVENC;
    case EncoderConfig::Backend::AMF: return EncoderBackend::AMF;
    case EncoderConfig::Backend::QSV: return EncoderBackend::QSV;
    case EncoderConfig::Backend::MF: return EncoderBackend::MF;
    case EncoderConfig::Backend::Auto:
    default: return EncoderBackend::Auto;
    }
}

EncoderConfig::Backend encoderConfigBackendFromString(const std::string& value)
{
    switch (encoderBackendFromString(value))
    {
    case EncoderBackend::NVENC: return EncoderConfig::Backend::NVENC;
    case EncoderBackend::AMF: return EncoderConfig::Backend::AMF;
    case EncoderBackend::QSV: return EncoderConfig::Backend::QSV;
    case EncoderBackend::MF: return EncoderConfig::Backend::MF;
    case EncoderBackend::Auto:
    default: return EncoderConfig::Backend::Auto;
    }
}

std::string encoderBackendToString(EncoderBackend backend)
{
    switch (backend)
    {
    case EncoderBackend::NVENC: return "nvenc";
    case EncoderBackend::AMF: return "amf";
    case EncoderBackend::QSV: return "qsv";
    case EncoderBackend::MF: return "mf";
    case EncoderBackend::Auto:
    default: return "auto";
    }
}

std::string encoderBackendDisplayName(EncoderBackend backend)
{
    switch (backend)
    {
    case EncoderBackend::NVENC: return "Nvidia NVENC";
    case EncoderBackend::AMF: return "AMD AMF";
    case EncoderBackend::QSV: return "Intel Quick Sync";
    case EncoderBackend::MF: return "Windows compatibility";
    case EncoderBackend::Auto:
    default: return "Auto";
    }
}

std::string codecToString(EncoderConfig::Codec codec)
{
    return codec == EncoderConfig::Codec::HEVC ? "HEVC" : "H.264";
}

std::string ffmpegEncoderName(EncoderBackend backend, EncoderConfig::Codec codec)
{
    const std::string prefix = codec == EncoderConfig::Codec::HEVC ? "hevc" : "h264";
    switch (backend)
    {
    case EncoderBackend::NVENC: return prefix + "_nvenc";
    case EncoderBackend::AMF: return prefix + "_amf";
    case EncoderBackend::QSV: return prefix + "_qsv";
    case EncoderBackend::MF: return prefix + "_mf";
    case EncoderBackend::Auto:
    default: return {};
    }
}

EncoderBackend preferredBackendForVendor(GpuVendor vendor)
{
    switch (vendor)
    {
    case GpuVendor::Nvidia: return EncoderBackend::NVENC;
    case GpuVendor::AMD: return EncoderBackend::AMF;
    case GpuVendor::Intel: return EncoderBackend::QSV;
    case GpuVendor::Unknown:
    default: return EncoderBackend::Auto;
    }
}

std::vector<EncoderCandidate> buildEncoderCandidates(
    EncoderBackend requested_backend,
    GpuVendor vendor,
    EncoderConfig::Codec preferred_codec,
    const std::set<std::string>& available_encoders)
{
    std::vector<EncoderCandidate> candidates;

    if (requested_backend != EncoderBackend::Auto)
    {
        appendBackendCandidates(candidates, requested_backend, preferred_codec, available_encoders);
        if (requested_backend != EncoderBackend::MF)
            appendCandidate(candidates, EncoderBackend::MF, EncoderConfig::Codec::H264, true, available_encoders);
        return candidates;
    }

    std::vector<EncoderBackend> ordered_backends;
    appendUniqueBackend(ordered_backends, preferredBackendForVendor(vendor));
    appendUniqueBackend(ordered_backends, EncoderBackend::NVENC);
    appendUniqueBackend(ordered_backends, EncoderBackend::AMF);
    appendUniqueBackend(ordered_backends, EncoderBackend::QSV);

    for (EncoderBackend backend : ordered_backends)
        appendBackendCandidates(candidates, backend, preferred_codec, available_encoders);

    appendCandidate(candidates, EncoderBackend::MF, EncoderConfig::Codec::H264, true, available_encoders);
    return candidates;
}
