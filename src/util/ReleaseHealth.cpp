#include "util/ReleaseHealth.h"
#include "capture/CaptureTarget.h"
#include "util/ClipProbe.h"
#include "encode/EncoderSelector.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>

#ifdef Q_OS_WIN
#  include <Windows.h>
#  include <dxgi1_2.h>
#  include <wrl/client.h>
#endif

namespace
{
ReleaseHealth::Check check(QString id, QString label, QString severity, QString message)
{
    return { std::move(id), std::move(label), std::move(severity), std::move(message) };
}

int clampInt(int value, int low, int high)
{
    return qBound(low, value, high);
}

QString findExecutableNearAppOrPath(const QString& name)
{
    const QString app_path = QDir(QCoreApplication::applicationDirPath()).filePath(name);
    if (QFileInfo::exists(app_path))
        return app_path;

    const QString in_path = QStandardPaths::findExecutable(name);
    if (!in_path.isEmpty())
        return in_path;

    const QString env_root = qEnvironmentVariable("FFMPEG_DIR");
    if (!env_root.isEmpty())
    {
        const QString env_path = QDir(env_root).filePath("bin/" + name);
        if (QFileInfo::exists(env_path))
            return env_path;
    }

    return QString();
}

#ifdef Q_OS_WIN
struct AdapterSummary
{
    bool anyAdapter = false;
    bool hasNvidia = false;
    bool hasAmd = false;
    bool hasIntel = false;
    QStringList names;
};

AdapterSummary queryAdapters()
{
    AdapterSummary summary;
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return summary;

    for (UINT i = 0;; ++i)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        if (FAILED(factory->EnumAdapters1(i, &adapter)))
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        if (FAILED(adapter->GetDesc1(&desc)))
            continue;

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        summary.anyAdapter = true;
        summary.hasNvidia = summary.hasNvidia || desc.VendorId == 0x10DE;
        summary.hasAmd = summary.hasAmd || desc.VendorId == 0x1002;
        summary.hasIntel = summary.hasIntel || desc.VendorId == 0x8086;
        summary.names.append(QString::fromWCharArray(desc.Description).trimmed());
    }

    return summary;
}
#endif
}

namespace ReleaseHealth
{
QString appVersion()
{
#ifdef GHOST_REPLAY_VERSION
    return QStringLiteral(GHOST_REPLAY_VERSION);
#else
    return QStringLiteral("0.1.0");
#endif
}

QString locateFfmpegExecutable()
{
#ifdef Q_OS_WIN
    return findExecutableNearAppOrPath("ffmpeg.exe");
#else
    return findExecutableNearAppOrPath("ffmpeg");
#endif
}

QVariantList toVariantList(const QList<Check>& checks)
{
    QVariantList list;
    for (const auto& item : checks)
    {
        QVariantMap map;
        map["id"] = item.id;
        map["label"] = item.label;
        map["severity"] = item.severity;
        map["message"] = item.message;
        list.append(map);
    }
    return list;
}

QList<Check> validateConfig(Config& config)
{
    QList<Check> checks;

    const auto addAdjusted = [&checks](const QString& label, const QString& message) {
        checks.append(check("config", label, "warning", message));
    };

    const std::string normalized_capture_source = normalizedCaptureSource(config.capture_source);
    if (config.capture_source != normalized_capture_source)
    {
        config.capture_source = normalized_capture_source;
        addAdjusted("Capture source", "Unsupported capture source reset to Desktop.");
    }

    const QString codec = QString::fromStdString(config.codec).toLower();
    if (codec == "h264" && config.codec != "h264")
    {
        config.codec = "h264";
        addAdjusted("Codec", "Capture codec name normalized to H.264.");
    }
    else if (codec != "h264")
    {
        config.codec = "h264";
        addAdjusted("Codec", "Capture codec reset to H.264 for this release.");
    }

    const EncoderBackend backend = encoderBackendFromString(config.encoder_backend);
    const std::string normalized_backend = encoderBackendToString(backend);
    if (config.encoder_backend != normalized_backend)
    {
        config.encoder_backend = normalized_backend;
        addAdjusted("Video encoder", "Unsupported encoder backend reset to Auto.");
    }

    const int oldCqp = config.cqp;
    config.cqp = clampInt(config.cqp, 0, 51);
    if (config.cqp != oldCqp)
        addAdjusted("Quality", "CQP was outside 0-51 and was clamped.");

    const int oldPreset = config.preset;
    config.preset = clampInt(config.preset, 1, 7);
    if (config.preset != oldPreset)
        addAdjusted("Preset", "Encoder preset was outside 1-7 and was clamped.");

    const int oldFps = config.fps;
    config.fps = clampInt(config.fps, 1, 240);
    if (config.fps != oldFps)
        addAdjusted("Frame rate", "FPS was outside 1-240 and was clamped.");

    const int oldKeyframes = config.keyframe_interval;
    config.keyframe_interval = clampInt(config.keyframe_interval, 1, 600);
    if (config.keyframe_interval != oldKeyframes)
        addAdjusted("Keyframes", "Keyframe interval was outside 1-600 and was clamped.");

    const int oldDuration = config.replay_duration_sec;
    config.replay_duration_sec = clampInt(config.replay_duration_sec, 15, 3600);
    if (config.replay_duration_sec != oldDuration)
        addAdjusted("Replay duration", "Replay duration was outside 15-3600 seconds and was clamped.");

    config.capture_width = qMax(0, config.capture_width);
    config.capture_height = qMax(0, config.capture_height);

    if (config.audio.channels != 1 && config.audio.channels != 2)
    {
        config.audio.channels = 2;
        addAdjusted("Audio channels", "Audio channels reset to stereo.");
    }

    if (config.audio.sample_rate != 44100 && config.audio.sample_rate != 48000 && config.audio.sample_rate != 96000)
    {
        config.audio.sample_rate = 48000;
        addAdjusted("Audio sample rate", "Audio sample rate reset to 48000 Hz.");
    }

    if (config.audio.codec != "aac")
    {
        config.audio.codec = "aac";
        addAdjusted("Audio codec", "Audio codec reset to AAC.");
    }

    const int oldBitrate = config.audio.bitrate_kbps;
    config.audio.bitrate_kbps = clampInt(config.audio.bitrate_kbps, 64, 320);
    if (config.audio.bitrate_kbps != oldBitrate)
        addAdjusted("Audio bitrate", "Audio bitrate was outside 64-320 kbps and was clamped.");

    if (config.output_dir.empty())
    {
        config.output_dir = "clips";
        addAdjusted("Output folder", "Empty output folder reset to clips.");
    }

    if ((config.hotkey.modifiers & 0xF) == 0)
    {
        config.hotkey.modifiers = 1;
        addAdjusted("Hotkey modifier", "Empty hotkey modifier reset to Alt.");
    }
    else if ((config.hotkey.modifiers & ~0xFu) != 0)
    {
        config.hotkey.modifiers &= 0xF;
        addAdjusted("Hotkey modifier", "Unsupported hotkey modifier bits were removed.");
    }

    if (config.hotkey.vk == 0 || config.hotkey.vk > 254)
    {
        config.hotkey.vk = 121;
        addAdjusted("Hotkey key", "Unsupported hotkey key reset to F10.");
    }

    if (checks.isEmpty())
        checks.append(check("config", "Configuration", "success", "Settings are in the supported beta range."));

    return checks;
}

QList<Check> collectStartupChecks(Config& config)
{
    QList<Check> checks = validateConfig(config);

#ifdef Q_OS_WIN
    const auto current = QOperatingSystemVersion::current();
    const bool win10OrNewer = current >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0);
    checks.append(check("os", "Windows", win10OrNewer ? "success" : "error",
        win10OrNewer ? "Windows 10 or newer detected." : "Ghost Replay requires Windows 10 1903 or newer."));

    const AdapterSummary adapters = queryAdapters();
    if (adapters.anyAdapter)
    {
        QStringList supported;
        if (adapters.hasNvidia)
            supported.append("Nvidia NVENC");
        if (adapters.hasAmd)
            supported.append("AMD AMF");
        if (adapters.hasIntel)
            supported.append("Intel Quick Sync");
        const QString encoder_note = supported.isEmpty()
            ? "Windows compatibility encoder will be attempted."
            : "Preferred encoders: " + supported.join(", ") + ".";
        checks.append(check("gpu", "GPU", "success",
            "Display adapters detected: " + adapters.names.join(", ") + ". " + encoder_note));
    }
    else
    {
        checks.append(check("gpu", "GPU", "error", "No hardware display adapter was detected."));
    }
#else
    checks.append(check("os", "Operating system", "error", "This beta supports recording on Windows only."));
    checks.append(check("gpu", "GPU", "error", "This beta requires Windows graphics capture support."));
#endif

    const QString ffmpeg = locateFfmpegExecutable();
    checks.append(check("ffmpeg", "FFmpeg", ffmpeg.isEmpty() ? "warning" : "success",
        ffmpeg.isEmpty() ? "ffmpeg was not found; share/export presets may be unavailable." : "ffmpeg found: " + ffmpeg));

    const QString ffprobe = ClipProbe::locateProbeExecutable();
    checks.append(check("ffprobe", "FFprobe", ffprobe.isEmpty() ? "warning" : "success",
        ffprobe.isEmpty() ? "ffprobe was not found; clip metadata and previews may be limited." : "ffprobe found: " + ffprobe));

    QDir output(QString::fromStdString(config.output_dir));
    bool outputOk = output.exists() || QDir().mkpath(output.absolutePath());
    if (outputOk)
    {
        const QString probePath = output.filePath(".ghostreplay-write-test");
        QFile probe(probePath);
        outputOk = probe.open(QIODevice::WriteOnly | QIODevice::Truncate);
        if (outputOk)
        {
            probe.write("ok");
            probe.close();
            QFile::remove(probePath);
        }
    }
    checks.append(check("output", "Output folder", outputOk ? "success" : "error",
        outputOk ? "Output folder is writable: " + output.absolutePath()
                 : "Output folder is not writable: " + output.absolutePath()));

    return checks;
}

QString diagnosticsText(const Config& config, const QList<Check>& checks, bool captureAvailable, const QString& captureError)
{
    QString text;
    QTextStream out(&text);

    out << "Ghost Replay diagnostics\n";
    out << "Version: " << appVersion() << "\n";
    out << "Qt: " << qVersion() << "\n";
    out << "OS: " << QSysInfo::prettyProductName() << "\n";
    out << "CPU arch: " << QSysInfo::currentCpuArchitecture() << "\n";
    out << "Capture available: " << (captureAvailable ? "yes" : "no") << "\n";
    if (!captureError.isEmpty())
        out << "Capture error: " << captureError << "\n";
    out << "Output dir: " << QString::fromStdString(config.output_dir) << "\n";
    out << "Capture source: " << QString::fromStdString(config.capture_source) << "\n";
    out << "Codec: " << QString::fromStdString(config.codec) << "\n";
    out << "Encoder backend: " << QString::fromStdString(config.encoder_backend) << "\n";
    out << "FPS: " << config.fps << "\n";
    out << "Replay duration: " << config.replay_duration_sec << "\n";
    out << "Audio enabled: " << (config.audio.enabled ? "yes" : "no") << "\n\n";
    out << "Checks:\n";
    for (const auto& item : checks)
        out << "- [" << item.severity << "] " << item.label << ": " << item.message << "\n";

    return text;
}
}
