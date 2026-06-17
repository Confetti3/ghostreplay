#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

#include "encode/EncoderSelector.h"

#include <vector>

class TranscodeJob : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        QString input_path;
        QString output_path;
        double trim_start_sec = 0.0;
        double trim_end_sec = 0.0;
        double source_duration_sec = 0.0;
        int source_width = 0;
        int source_height = 0;
        bool has_audio = false;
        int target_width = 0;
        int target_height = 0;
        int target_fps = 0;
        int quality = 70;
        bool fit_to_size = false;
        double target_size_mb = 0.0;
        int audio_bitrate_kbps = 128;
        QString export_mode = "compressed";
        QString codec = "h264";
        QString encoder_backend = "auto";
    };

    explicit TranscodeJob(Options options, QObject* parent = nullptr);

    void start();
    void cancel();
    bool isRunning() const;

signals:
    void progressChanged(double progress, const QString& status);
    void finished(bool success, const QString& outputPath, const QString& message);

private:
    void startAttempt();
    void onStdOutReady();
    void onStdErrReady();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    qint64 computeTargetVideoBitrate() const;
    bool isLosslessMode() const;
    EncoderConfig::Codec requestedCodec() const;
    const EncoderCandidate& currentCandidate() const;
    void buildEncoderCandidates();
    QString videoEncoder() const;
    QString locateFfmpegExecutable() const;
    QString outputSummary() const;

    Options m_options;
    QProcess* m_process = nullptr;
    QString m_stdout_buffer;
    QString m_stderr_buffer;
    QString m_last_error;
    int m_attempt = 0;
    int m_candidate_index = 0;
    qint64 m_current_video_bitrate = 0;
    bool m_cancelled = false;
    std::vector<EncoderCandidate> m_candidates;
};
