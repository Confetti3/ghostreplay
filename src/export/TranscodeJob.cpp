#include "export/TranscodeJob.h"

#include "util/ClipProbe.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <utility>

#ifdef Q_OS_WIN
#  include <Windows.h>
#endif

namespace
{
QString formatSeconds(double value)
{
    return QString::number(value, 'f', 3);
}

QString nvencPresetForQuality(int quality)
{
    if (quality >= 85)
        return "p7";
    if (quality >= 65)
        return "p5";
    return "p3";
}

QString amfPresetForQuality(int quality)
{
    if (quality >= 85)
        return "quality";
    if (quality >= 60)
        return "balanced";
    return "speed";
}

QString qsvPresetForQuality(int quality)
{
    if (quality >= 85)
        return "veryslow";
    if (quality >= 65)
        return "medium";
    return "veryfast";
}

void appendEncoderOptions(QStringList& args, const EncoderCandidate& candidate, int quality)
{
    switch (candidate.backend)
    {
    case EncoderBackend::NVENC:
        args << "-preset" << nvencPresetForQuality(quality)
             << "-tune" << "hq";
        break;
    case EncoderBackend::AMF:
        args << "-usage" << "high_quality"
             << "-preset" << amfPresetForQuality(quality);
        break;
    case EncoderBackend::QSV:
        args << "-preset" << qsvPresetForQuality(quality);
        break;
    case EncoderBackend::MF:
        args << "-rate_control" << "quality"
             << "-quality" << QString::number(qBound(1, quality, 100))
             << "-hw_encoding" << "0";
        break;
    case EncoderBackend::Auto:
    default:
        break;
    }
}

void hideProcessWindow(QProcess& process)
{
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags &= ~CREATE_NEW_CONSOLE;
        args->flags |= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
    });
#else
    Q_UNUSED(process);
#endif
}
}

TranscodeJob::TranscodeJob(Options options, QObject* parent)
    : QObject(parent)
    , m_options(std::move(options))
{}

void TranscodeJob::start()
{
    if (m_process)
        return;

    m_current_video_bitrate = computeTargetVideoBitrate();
    buildEncoderCandidates();
    startAttempt();
}

void TranscodeJob::cancel()
{
    m_cancelled = true;
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

bool TranscodeJob::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

QString TranscodeJob::locateFfmpegExecutable() const
{
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QString local = QDir(app_dir).filePath("ffmpeg.exe");
    if (QFileInfo::exists(local))
        return local;

    const QString in_path = QStandardPaths::findExecutable("ffmpeg");
    if (!in_path.isEmpty())
        return in_path;

    const QString env_root = qEnvironmentVariable("FFMPEG_DIR");
    if (!env_root.isEmpty())
    {
        const QString env_path = QDir(env_root).filePath("bin/ffmpeg.exe");
        if (QFileInfo::exists(env_path))
            return env_path;
    }

    return QString();
}

qint64 TranscodeJob::computeTargetVideoBitrate() const
{
    const double duration = qMax(0.5, m_options.trim_end_sec - m_options.trim_start_sec);
    const qint64 audio_bps = m_options.has_audio ? static_cast<qint64>(m_options.audio_bitrate_kbps) * 1000 : 0;

    if (m_options.fit_to_size && m_options.target_size_mb > 0.0)
    {
        const double total_bits = m_options.target_size_mb * 1024.0 * 1024.0 * 8.0 * 0.92;
        const qint64 total_bps = static_cast<qint64>(total_bits / duration);
        return qMax<qint64>(350000, total_bps - audio_bps);
    }

    const int width = m_options.target_width > 0 ? m_options.target_width : qMax(1280, m_options.source_width);
    const int height = m_options.target_height > 0 ? m_options.target_height : qMax(720, m_options.source_height);
    const int fps = m_options.target_fps > 0 ? m_options.target_fps : 60;
    const double scale = (static_cast<double>(width) * static_cast<double>(height) * fps) /
        (1280.0 * 720.0 * 60.0);
    const double quality_scale = 1.6 + (qBound(0, m_options.quality, 100) / 100.0) * 5.4;
    return static_cast<qint64>(qBound(700000.0, scale * quality_scale * 1000000.0, 24000000.0));
}

bool TranscodeJob::isLosslessMode() const
{
    return m_options.export_mode == "lossless_trim";
}

EncoderConfig::Codec TranscodeJob::requestedCodec() const
{
    return m_options.codec.compare("hevc", Qt::CaseInsensitive) == 0
        ? EncoderConfig::Codec::HEVC
        : EncoderConfig::Codec::H264;
}

void TranscodeJob::buildEncoderCandidates()
{
    m_candidates = ::buildEncoderCandidates(
        encoderBackendFromString(m_options.encoder_backend.toStdString()),
        GpuVendor::Unknown,
        requestedCodec());
    if (m_candidates.empty())
    {
        EncoderCandidate fallback;
        fallback.backend = EncoderBackend::MF;
        fallback.codec = EncoderConfig::Codec::H264;
        fallback.encoder_name = "h264_mf";
        fallback.display_name = "Windows compatibility H.264";
        fallback.compatibility_fallback = true;
        m_candidates.push_back(std::move(fallback));
    }
    m_candidate_index = 0;
}

const EncoderCandidate& TranscodeJob::currentCandidate() const
{
    return m_candidates.at(static_cast<size_t>(qBound(0, m_candidate_index, static_cast<int>(m_candidates.size()) - 1)));
}

QString TranscodeJob::videoEncoder() const
{
    return QString::fromStdString(currentCandidate().encoder_name);
}

void TranscodeJob::startAttempt()
{
    const QString ffmpeg = locateFfmpegExecutable();
    if (ffmpeg.isEmpty())
    {
        emit finished(false, QString(), "FFmpeg executable was not found next to the app or in PATH.");
        return;
    }

    QFileInfo out_info(m_options.output_path);
    QDir().mkpath(out_info.absolutePath());
    QFile::remove(m_options.output_path);

    if (m_process)
        m_process->deleteLater();
    m_process = new QProcess(this);
    hideProcessWindow(*m_process);
    m_stdout_buffer.clear();
    m_stderr_buffer.clear();
    m_last_error.clear();
    ++m_attempt;

    QStringList args{
        "-y",
        "-v", "error",
        "-progress", "pipe:1",
        "-nostats",
        "-ss", formatSeconds(m_options.trim_start_sec),
        "-t", formatSeconds(qMax(0.1, m_options.trim_end_sec - m_options.trim_start_sec)),
        "-i", m_options.input_path
    };

    if (isLosslessMode())
    {
        args << "-map_metadata" << "-1"
             << "-map" << "0:v:0"
             << "-map" << "0:a:0?"
             << "-c" << "copy"
             << "-avoid_negative_ts" << "make_zero"
             << "-movflags" << "+faststart"
             << m_options.output_path;

        connect(m_process, &QProcess::readyReadStandardOutput, this, &TranscodeJob::onStdOutReady);
        connect(m_process, &QProcess::readyReadStandardError, this, &TranscodeJob::onStdErrReady);
        connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                this, &TranscodeJob::onProcessFinished);

        emit progressChanged(0.0, "Trimming clip without re-encoding");
        m_process->start(ffmpeg, args);
        return;
    }

    QStringList filters;
    if (m_options.target_width > 0 && m_options.target_height > 0)
    {
        filters << QString("scale=%1:%2:force_original_aspect_ratio=decrease")
            .arg(m_options.target_width)
            .arg(m_options.target_height);
        filters << QString("pad=%1:%2:(ow-iw)/2:(oh-ih)/2:black")
            .arg(m_options.target_width)
            .arg(m_options.target_height);
    }
    if (m_options.target_fps > 0)
        filters << QString("fps=%1").arg(m_options.target_fps);

    if (!filters.isEmpty())
    {
        args << "-vf" << filters.join(',');
    }

    const EncoderCandidate& candidate = currentCandidate();

    args << "-map_metadata" << "-1"
         << "-map" << "0:v:0"
         << "-c:v" << videoEncoder();

    appendEncoderOptions(args, candidate, m_options.quality);

    args << "-pix_fmt" << "yuv420p"
         << "-b:v" << QString::number(m_current_video_bitrate)
         << "-maxrate" << QString::number(static_cast<qint64>(m_current_video_bitrate * 1.15))
         << "-bufsize" << QString::number(static_cast<qint64>(m_current_video_bitrate * 2));

    if (m_options.has_audio)
    {
        args << "-map" << "0:a:0?"
             << "-ac" << "2"
             << "-ar" << "48000";
        args << "-c:a" << "aac"
             << "-b:a" << QString("%1k").arg(m_options.audio_bitrate_kbps);
    }
    else
    {
        args << "-an";
    }

    args << "-movflags" << "+faststart"
         << m_options.output_path;

    connect(m_process, &QProcess::readyReadStandardOutput, this, &TranscodeJob::onStdOutReady);
    connect(m_process, &QProcess::readyReadStandardError, this, &TranscodeJob::onStdErrReady);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &TranscodeJob::onProcessFinished);

    emit progressChanged(0.0, QString("Exporting clip with %1 (pass %2)")
        .arg(QString::fromStdString(candidate.display_name))
        .arg(m_attempt));
    m_process->start(ffmpeg, args);
}

void TranscodeJob::onStdOutReady()
{
    m_stdout_buffer += QString::fromUtf8(m_process->readAllStandardOutput());
    while (true)
    {
        const int newline = m_stdout_buffer.indexOf('\n');
        if (newline < 0)
            break;

        const QString line = m_stdout_buffer.left(newline).trimmed();
        m_stdout_buffer.remove(0, newline + 1);

        if (line.startsWith("out_time_ms="))
        {
            bool ok = false;
            const double out_time_us = line.mid(QString("out_time_ms=").size()).toDouble(&ok);
            if (ok)
            {
                const double duration_us = qMax(100000.0, (m_options.trim_end_sec - m_options.trim_start_sec) * 1000000.0);
                const double progress = qBound(0.0, out_time_us / duration_us, 1.0);
                emit progressChanged(progress, QString("Exporting clip (%1%)").arg(qRound(progress * 100.0)));
            }
        }
    }
}

void TranscodeJob::onStdErrReady()
{
    m_stderr_buffer += QString::fromUtf8(m_process->readAllStandardError());
}

QString TranscodeJob::outputSummary() const
{
    const QFileInfo out_info(m_options.output_path);
    return QString("Saved share clip to %1 (%2)")
        .arg(out_info.fileName(), ClipProbe::formatSize(out_info.size()));
}

void TranscodeJob::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool normal_exit = exitStatus == QProcess::NormalExit && exitCode == 0;
    if (!normal_exit)
    {
        if (!m_cancelled && !isLosslessMode() && m_candidate_index + 1 < static_cast<int>(m_candidates.size()))
        {
            ++m_candidate_index;
            emit progressChanged(0.0, QString("Retrying export with %1")
                .arg(QString::fromStdString(currentCandidate().display_name)));
            startAttempt();
            return;
        }

        const QString error = m_cancelled
            ? "Export cancelled."
            : (m_stderr_buffer.trimmed().isEmpty() ? "FFmpeg export failed." : m_stderr_buffer.trimmed());
        QFile::remove(m_options.output_path);
        emit finished(false, QString(), error);
        m_process->deleteLater();
        m_process = nullptr;
        return;
    }

    if (m_options.fit_to_size && m_options.target_size_mb > 0.0)
    {
        const QFileInfo out_info(m_options.output_path);
        const qint64 target_bytes = static_cast<qint64>(m_options.target_size_mb * 1024.0 * 1024.0);
        if (out_info.exists() && out_info.size() > target_bytes && m_attempt < 3)
        {
            m_current_video_bitrate = qMax<qint64>(250000, static_cast<qint64>(m_current_video_bitrate * 0.82));
            emit progressChanged(0.0, QString("Refining file size (pass %1)").arg(m_attempt + 1));
            startAttempt();
            return;
        }
    }

    emit progressChanged(1.0, "Export complete");
    emit finished(true, m_options.output_path, outputSummary());
    m_process->deleteLater();
    m_process = nullptr;
}
