#include "ui/GRApplication.h"
#include "ui/Backend.h"
#include "ui/SettingsWindow.h"
#include "capture/CaptureManager.h"
#include "buffer/RingBuffer.h"
#include "audio/AudioCapture.h"
#include "hotkey/HotkeyManager.h"
#include "export/ClipExporter.h"
#include "export/TranscodeJob.h"
#include "game/GameDetector.h"
#include "encode/EncoderConfig.h"
#include "encode/EncoderSelector.h"
#include "util/Config.h"
#include "util/ClipProbe.h"
#include "util/FileActions.h"
#include "util/Logger.h"
#include "util/ReleaseHealth.h"

#include <QTimer>
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QWindow>
#include <iostream>

#ifdef Q_OS_WIN
#  include <Windows.h>
#endif

static QIcon appIcon()
{
    return QIcon(":/branding/ghost-loop.png");
}

// ── Constructor ────────────────────────────────────────────────

GRApplication::GRApplication(Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
}

GRApplication::~GRApplication()
{
    shutdown();
}

// ── Initialize the full pipeline ───────────────────────────────

void GRApplication::connectBackendActions()
{
    if (!m_backend || m_backend_connected)
        return;

    m_backend_connected = true;
    connect(m_backend, &Backend::saveClipRequested,
            this, &GRApplication::onSaveClip);
    connect(m_backend, &Backend::exportClipRequested,
            this, &GRApplication::onExportClip);
    connect(m_backend, &Backend::cancelExportRequested,
            this, &GRApplication::onCancelExport);
    connect(m_backend, &Backend::toggleRecordingRequested,
            this, &GRApplication::onToggleRecording);
    connect(m_backend, &Backend::openSettingsRequested,
            this, &GRApplication::onOpenSettings);
    connect(m_backend, &Backend::openOutputDirRequested,
            this, [this]() {
                QDir dir(QString::fromStdString(m_config.output_dir));
                if (!dir.exists() && !QDir().mkpath(dir.absolutePath()))
                {
                    m_backend->reportError("Could not create clips folder: " + dir.absolutePath());
                    return;
                }

                if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath())))
                    m_backend->reportError("Could not open clips folder: " + dir.absolutePath());
            });
    connect(m_backend, &Backend::openClipRequested,
            this, &GRApplication::onOpenClip);
    connect(m_backend, &Backend::deleteClipRequested,
            this, &GRApplication::onDeleteClip);
    connect(m_backend, &Backend::revealInExplorerRequested,
            this, &GRApplication::onRevealInExplorer);
    connect(m_backend, &Backend::hideToTrayRequested,
            this, &GRApplication::hideMainWindowToTray);
    connect(m_backend, &Backend::refreshRequested,
            this, &GRApplication::refreshClipLibrary);
    connect(m_backend, &Backend::retryCaptureRequested,
            this, &GRApplication::onRetryCapture);
    connect(m_backend, &Backend::openLogFileRequested,
            this, &GRApplication::onOpenLogFile);
    connect(m_backend, &Backend::copyClipToClipboardRequested,
            this, &GRApplication::onCopyClipToClipboard);
    connect(m_backend, &Backend::copyDiagnosticsRequested,
            this, &GRApplication::onCopyDiagnostics);
    connect(m_backend, &Backend::settingsSaved,
            this, &GRApplication::onSettingsSaved);
}

void GRApplication::updateHealthChecks()
{
    const auto checks = ReleaseHealth::collectStartupChecks(m_config);
    m_health_checks = ReleaseHealth::toVariantList(checks);
    if (m_backend)
    {
        m_backend->setHealthChecks(m_health_checks);
        m_backend->setConfig(&m_config);
    }
}

void GRApplication::setCaptureUnavailable(const QString& reason)
{
    stopCapturePipeline();
    m_capture_available = false;
    m_recording = false;
    m_capture_error = reason;

    if (m_backend)
    {
        m_backend->setRecording(false);
        m_backend->setAudioActive(false);
        m_backend->setCaptureStatus(false, reason);
    }

    if (m_action_status)
        m_action_status->setText("Status: Capture unavailable");
    if (m_action_save)
        m_action_save->setEnabled(false);
    if (m_action_toggle)
    {
        m_action_toggle->setText("Retry Capture");
        m_action_toggle->setEnabled(true);
    }
    if (m_tray)
        m_tray->setToolTip("GhostReplay - Capture unavailable");

    LOG_ERROR("GRApplication: capture unavailable: " + reason.toStdString());
}

bool GRApplication::startCapturePipeline(bool allowInteractiveWindowPicker)
{
    stopCapturePipeline();
    updateHealthChecks();

    for (const QVariant& item : m_health_checks)
    {
        const QVariantMap map = item.toMap();
        if (map.value("severity").toString() == "error")
        {
            setCaptureUnavailable(map.value("message").toString());
            return false;
        }
    }

    if (m_config.capture_source == "window" && !allowInteractiveWindowPicker)
    {
        setCaptureUnavailable("Window capture needs a selected window. Click Retry Capture to open the Windows picker, or switch to Desktop/Game in Settings.");
        return false;
    }

    m_enc_cfg = std::make_unique<EncoderConfig>();
    m_enc_cfg->qp_p       = m_config.cqp;
    m_enc_cfg->qp_i       = m_config.cqp;
    m_enc_cfg->gop_length = m_config.keyframe_interval;
    m_enc_cfg->fps_num    = m_config.fps;
    m_enc_cfg->fps_den    = 1;
    m_enc_cfg->width      = m_config.capture_width;
    m_enc_cfg->height     = m_config.capture_height;
    m_enc_cfg->preset     = m_config.preset;
    m_enc_cfg->backend    = encoderConfigBackendFromString(m_config.encoder_backend);
    if (m_config.codec == "hevc")
        m_enc_cfg->codec = EncoderConfig::Codec::HEVC;

    m_capture_mgr = std::make_unique<CaptureManager>();
    void* owner_window = nullptr;
#ifdef Q_OS_WIN
    if (m_main_window)
        owner_window = reinterpret_cast<void*>(m_main_window->winId());
#endif

    if (!m_capture_mgr->initialize(m_config.monitor_index, *m_enc_cfg,
                                   m_config.capture_source, owner_window,
                                   allowInteractiveWindowPicker))
    {
        setCaptureUnavailable("Capture initialization failed. Check that the selected capture source is available and at least one supported encoder can start.");
        return false;
    }

    const int64_t max_dur = static_cast<int64_t>(m_config.replay_duration_sec) * 1'000'000LL;
    m_ring_buffer = std::make_unique<RingBuffer>(max_dur);

    if (m_config.audio.enabled)
    {
        m_audio_cap = std::make_unique<AudioCapture>();
        if (m_audio_cap->initialize(m_config.audio))
        {
            m_audio_cap->start();
            m_audio_active = true;
            LOG_INFO("GRApplication: audio capture started");
        }
        else
        {
            m_audio_active = false;
            LOG_WARNING("GRApplication: audio init failed - continuing video-only");
        }
    }

    m_capture_mgr->start(*m_ring_buffer);
    m_recording = true;
    m_capture_available = true;
    m_capture_error.clear();

    rebindHotkey();

    if (m_backend)
    {
        m_backend->setRecording(true);
        m_backend->setAudioActive(m_audio_active);
        m_backend->setCaptureStatus(true, QString());
        m_backend->setCurrentGame(QString::fromStdString(m_capture_mgr->captureTargetName()));
    }

    if (m_action_save)
        m_action_save->setEnabled(true);
    if (m_action_toggle)
    {
        m_action_toggle->setEnabled(true);
        m_action_toggle->setText("Pause Recording");
    }
    updateTrayTooltip();
    return true;
}

void GRApplication::stopCapturePipeline()
{
    if (m_hotkey_mgr)
    {
        m_hotkey_mgr->stop();
        m_hotkey_mgr.reset();
    }

    if (m_audio_active && m_audio_cap)
        m_audio_cap->stop();
    m_audio_active = false;
    m_audio_cap.reset();

    if (m_capture_mgr)
        m_capture_mgr->stop();
    m_capture_mgr.reset();
    m_ring_buffer.reset();
    m_enc_cfg.reset();
}

void GRApplication::rebindHotkey()
{
    if (m_hotkey_mgr)
    {
        m_hotkey_mgr->stop();
        m_hotkey_mgr.reset();
    }

    if (!m_capture_available)
        return;

    m_hotkey_mgr = std::make_unique<HotkeyManager>();
    if (!m_hotkey_mgr->registerHotkey(
            m_config.hotkey.modifiers,
            m_config.hotkey.vk,
            [this]() { onSaveClip(); }))
    {
        LOG_WARNING("GRApplication: hotkey registration failed");
    }
}

void GRApplication::applySettings(bool captureRestartRequired)
{
    if (!captureRestartRequired)
    {
        updateHealthChecks();
        rebindHotkey();
        if (m_backend)
            m_backend->reportSuccess("Settings saved. Changes are live.");
        return;
    }

    const bool should_resume_recording = m_recording;
    const bool was_available = m_capture_available;

    if (m_backend)
        m_backend->reportInfo("Applying capture changes. Recording will restart and the replay buffer will reset.");

    if (!startCapturePipeline(true))
    {
        if (m_backend)
        {
            const QString message = was_available
                ? "Settings were saved, but the new capture configuration could not start."
                : "Settings were saved, but capture is still unavailable.";
            m_backend->reportError(m_capture_error.isEmpty() ? message : m_capture_error);
        }
        return;
    }

    if (!should_resume_recording && m_capture_available)
    {
        m_capture_mgr->stop();
        if (m_audio_active && m_audio_cap)
            m_audio_cap->stop();
        m_recording = false;
        if (m_action_toggle)
            m_action_toggle->setText("Resume Recording");
        if (m_backend)
        {
            m_backend->setRecording(false);
            m_backend->setAudioActive(false);
        }
        updateTrayTooltip();
    }

    if (m_backend)
        m_backend->reportSuccess("Settings saved. Capture changes are live.");
}

bool GRApplication::initialize()
{
    {
        QFile themeFile(":/themes/dark.qss");
        if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qApp->setStyleSheet(themeFile.readAll());
            themeFile.close();
            LOG_INFO("GRApplication: dark theme loaded");
        }
    }

    if (!m_backend)
    {
        LOG_ERROR("GRApplication: backend not set before initialize()");
        return false;
    }

    m_backend->setConfig(&m_config);
    connectBackendActions();
    updateHealthChecks();
    m_backend->setRecording(false);
    m_backend->setAudioActive(false);
    m_backend->setCurrentGame("Desktop");
    m_backend->setExportState(false, 0.0, QString());
    m_backend->setCaptureStatus(false, QString());
    refreshClipLibrary();

    createTrayIcon();
    QTimer::singleShot(0, this, [this]()
    {
        startCapturePipeline(false);
    });

    auto* status_timer = new QTimer(this);
    connect(status_timer, &QTimer::timeout, this, &GRApplication::onStatusUpdate);
    status_timer->start(3000);

    return true;
}

// ── Shutdown ────────────────────────────────────────────────────

void GRApplication::shutdown()
{
    stopCapturePipeline();

    LOG_INFO("GRApplication: shutdown complete");
}

// ── Create tray icon and menu ──────────────────────────────────

void GRApplication::createTrayIcon()
{
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(appIcon());
    m_tray->setToolTip("GhostReplay — Recording");

    m_tray_menu = new QMenu();

    m_action_status = m_tray_menu->addAction("Status: Recording");
    m_action_status->setEnabled(false);

    m_tray_menu->addSeparator();

    QAction* show_action = m_tray_menu->addAction("Show Ghost Replay");
    connect(show_action, &QAction::triggered, this, &GRApplication::showMainWindow);

    m_tray_menu->addSeparator();

    m_action_save = m_tray_menu->addAction("Save Clip\tAlt+F10");
    connect(m_action_save, &QAction::triggered, this, &GRApplication::onSaveClip);

    m_action_toggle = m_tray_menu->addAction("Pause Recording");
    connect(m_action_toggle, &QAction::triggered, this, &GRApplication::onToggleRecording);

    m_tray_menu->addSeparator();

    QAction* exit_action = m_tray_menu->addAction("Exit");
    connect(exit_action, &QAction::triggered, this, &GRApplication::onExit);

    m_tray->setContextMenu(m_tray_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason)
            {
                if (reason == QSystemTrayIcon::Trigger
                    || reason == QSystemTrayIcon::DoubleClick)
                    showMainWindow();
            });

    m_tray->show();
}

void GRApplication::showMainWindow()
{
    if (!m_main_window)
        return;

    if (m_main_window->windowStates() & Qt::WindowMinimized)
        m_main_window->showNormal();
    else
        m_main_window->show();

    m_main_window->raise();
    m_main_window->requestActivate();
}

void GRApplication::hideMainWindowToTray()
{
    if (!m_main_window)
        return;

    m_main_window->hide();
    if (!m_tray_hint_shown && m_tray)
    {
        m_tray_hint_shown = true;
        m_tray->showMessage("GhostReplay",
            "Ghost Replay is still running. Use the tray icon to show it again.",
            QSystemTrayIcon::Information, 4000);
    }
}

bool GRApplication::runTrayRestoreSmokeTest()
{
    if (!m_main_window)
        return false;

    m_main_window->show();
    QCoreApplication::processEvents();

    hideMainWindowToTray();
    QCoreApplication::processEvents();
    const bool hidden_to_tray = !m_main_window->isVisible();

    showMainWindow();
    QCoreApplication::processEvents();
    const bool restored = m_main_window->isVisible()
        && !(m_main_window->windowStates() & Qt::WindowMinimized);

    return hidden_to_tray && restored;
}

// ── Update tray tooltip with buffer info ────────────────────────

void GRApplication::updateTrayTooltip()
{
    if (!m_tray) return;

    if (!m_capture_available || !m_ring_buffer)
    {
        m_tray->setToolTip("GhostReplay - Capture unavailable");
        if (m_action_status)
            m_action_status->setText("Status: Capture unavailable");
        return;
    }

    int64_t dur_sec = m_ring_buffer->duration() / 1'000'000;
    int pkt_count = static_cast<int>(m_ring_buffer->packetCount());

    QString tip = QString("GhostReplay — %1\n%2 packets | ~%3s cached")
        .arg(m_recording ? "Recording" : "Paused")
        .arg(pkt_count)
        .arg(dur_sec);

    m_tray->setToolTip(tip);
    m_action_status->setText(m_recording ? "Status: Recording" : "Status: Paused");
}

// ── Slot: Timer fires → update tray + backend ───────────────────

void GRApplication::onStatusUpdate()
{
    if (!m_backend)
        return;

    updateTrayTooltip();
    if (m_capture_available && m_config.capture_source == "desktop")
        pollGameDetection();

    if (m_ring_buffer)
    {
        m_backend->setBufferPackets(static_cast<int>(m_ring_buffer->packetCount()));
        m_backend->setBufferSeconds(static_cast<int>(m_ring_buffer->duration() / 1'000'000));
    }
}

// ── Poll foreground game ──────────────────────────────────────

void GRApplication::pollGameDetection()
{
    std::string game = GameDetector::currentGame();
    if (game != m_current_game)
    {
        m_current_game = game;
        LOG_INFO("GRApplication: game changed to '" + game + "'");
        m_backend->setCurrentGame(QString::fromStdString(game));
    }
}

// ── Slot: Save clip (hotkey or menu) ────────────────────────────

void GRApplication::onSaveClip()
{
    if (!m_ring_buffer || !m_capture_mgr)
    {
        m_backend->reportError(m_capture_error.isEmpty()
            ? "Capture is unavailable."
            : m_capture_error);
        return;
    }

    if (m_ring_buffer->packetCount() == 0)
    {
        LOG_WARNING("GRApplication: buffer empty — nothing to save");
        QString msg = "Buffer is empty — nothing to save.";
        m_tray->showMessage("GhostReplay", msg, QSystemTrayIcon::Warning, 3000);
        m_backend->reportError(msg);
        return;
    }

    performExport();
}

// ── Actual export (flush + mux) ───────────────────────────────

void GRApplication::performExport()
{
    int64_t max_dur = static_cast<int64_t>(m_config.replay_duration_sec) * 1'000'000LL;
    int64_t latest_pts = m_ring_buffer->latestPts();
    int64_t window_start = (latest_pts > max_dur) ? (latest_pts - max_dur) : 0;

    std::vector<RingBufferPacket> video_packets;
    const int64_t actual_start_us = m_ring_buffer->flushWindow(window_start, video_packets);

    if (video_packets.empty())
    {
        LOG_WARNING("GRApplication: no video packets after flush");
        return;
    }

    std::vector<EncodedAudioPacket> audio_packets;
    if (m_audio_active && m_audio_cap)
        m_audio_cap->flushWindow(actual_start_us, audio_packets);

    for (auto& pkt : video_packets)
    {
        pkt.pts = pkt.pts >= actual_start_us ? pkt.pts - actual_start_us : 0;
        pkt.dts = pkt.dts >= actual_start_us ? pkt.dts - actual_start_us : 0;
    }

    if (!audio_packets.empty() && m_config.audio.sample_rate > 0)
    {
        const int64_t audio_start_samples = actual_start_us * m_config.audio.sample_rate / 1'000'000LL;
        for (auto& pkt : audio_packets)
            pkt.pts_samples = pkt.pts_samples >= audio_start_samples ? pkt.pts_samples - audio_start_samples : 0;
    }

    std::string output_path = ClipExporter::generateFilename(m_config.output_dir, m_current_game);
    auto seq_headers = m_capture_mgr->encoder()->sequenceHeaders();

    ClipExporter exporter;

    if (m_audio_active && !audio_packets.empty())
    {
        auto a_extra = m_audio_cap->codecConfig();
        if (!exporter.initialize(output_path, seq_headers,
                                  m_capture_mgr->width(), m_capture_mgr->height(),
                                  m_config.fps, 1,
                                  a_extra, m_config.audio.sample_rate,
                                  m_config.audio.channels))
        {
            LOG_ERROR("GRApplication: ClipExporter init (audio) failed");
            m_backend->reportError("Export failed: encoder init error.");
            return;
        }
    }
    else
    {
        if (!exporter.initialize(output_path, seq_headers,
                                  m_capture_mgr->width(), m_capture_mgr->height(),
                                  m_config.fps, 1))
        {
            LOG_ERROR("GRApplication: ClipExporter init failed");
            m_backend->reportError("Export failed: encoder init error.");
            return;
        }
    }

    if (!exporter.writeVideoPackets(video_packets))
    {
        LOG_ERROR("GRApplication: writeVideoPackets failed");
        m_backend->reportError("Export failed: video write error.");
        return;
    }

    if (!audio_packets.empty() && !exporter.writeAudioPackets(audio_packets))
    {
        LOG_ERROR("GRApplication: writeAudioPackets failed");
        m_backend->reportError("Export failed: audio write error.");
        return;
    }

    if (!exporter.finalize())
    {
        LOG_ERROR("GRApplication: finalize failed");
        m_backend->reportError("Export failed: finalize error.");
        return;
    }

    int64_t duration_ms = 0;
    if (!video_packets.empty())
        duration_ms = (video_packets.back().pts - video_packets.front().pts) / 1000;
    double duration_sec = duration_ms / 1000.0;
    int frame_count = static_cast<int>(video_packets.size());

    QString clipPath = QString::fromStdString(output_path);
    QString msg = QString("Saved %1 (%2 frames, %3s)")
        .arg(clipPath).arg(frame_count).arg(duration_sec, 0, 'f', 1);

    m_tray->showMessage("GhostReplay", msg, QSystemTrayIcon::Information, 5000);
    LOG_INFO("GRApplication: exported " + output_path);

    // Update backend
    QDateTime now = QDateTime::currentDateTime();
    QFileInfo fi(clipPath);
    m_backend->addRecentClip(clipPath, fi.fileName(),
                              now.toString("hh:mm:ss"));
    refreshClipLibrary();
}

// ── Refresh clip library → push to Backend ──────────────────

void GRApplication::refreshClipLibrary()
{
    QVariantList list;
    QDir dir(QString::fromStdString(m_config.output_dir));
    if (!dir.exists())
        QDir().mkpath(dir.absolutePath());
    QFileInfoList files = dir.entryInfoList({"*.mp4"}, QDir::Files, QDir::Time);

    for (const auto& fi : files)
    {
        if (auto metadata = ClipProbe::probe(fi.absoluteFilePath()))
            list.append(ClipProbe::toVariantMap(*metadata));
    }
    m_backend->setClipLibrary(list);
}

void GRApplication::onExportClip(const QVariantMap& options)
{
    if (m_export_job)
    {
        m_backend->reportError("An export is already running.");
        return;
    }

    const QString input_path = options.value("path").toString();
    if (input_path.isEmpty())
    {
        m_backend->reportError("No clip selected for export.");
        return;
    }

    if (!FileActions::isValidLocalFile(input_path))
    {
        m_backend->reportError("Selected clip no longer exists.");
        return;
    }
    const QFileInfo source_info(input_path);

    const auto metadata = ClipProbe::probe(input_path);
    const double source_duration = metadata ? metadata->duration_sec : options.value("durationSec").toDouble();
    const double trim_start = qMax(0.0, options.value("trimStartSec").toDouble());
    const double trim_end = qMax(trim_start + 0.1, options.value("trimEndSec").toDouble());
    QString export_mode = options.value("exportMode").toString();
    if (export_mode.isEmpty())
        export_mode = "compressed";
    const QSet<QString> valid_modes{ "lossless_trim", "original_quality", "compressed", "discord_25mb", "twitter_512mb" };
    if (!valid_modes.contains(export_mode))
        export_mode = "compressed";

    QString base_name = options.value("outputName").toString().trimmed();
    if (base_name.isEmpty())
        base_name = source_info.completeBaseName() + "_share";
    base_name = FileActions::sanitizeFileBaseName(base_name);

    const QString output_dir = QString::fromStdString(m_config.output_dir);
    const QString output_path = FileActions::uniqueFilePath(output_dir, base_name, ".mp4");

    TranscodeJob::Options job_options;
    job_options.input_path = input_path;
    job_options.output_path = output_path;
    job_options.trim_start_sec = trim_start;
    job_options.trim_end_sec = qMin(trim_end, source_duration > 0.0 ? source_duration : trim_end);
    job_options.source_duration_sec = source_duration;
    job_options.source_width = metadata ? metadata->width : options.value("width").toInt();
    job_options.source_height = metadata ? metadata->height : options.value("height").toInt();
    job_options.has_audio = metadata ? metadata->has_audio : options.value("hasAudio").toBool();
    job_options.target_width = options.value("targetWidth").toInt();
    job_options.target_height = options.value("targetHeight").toInt();
    job_options.target_fps = options.value("targetFps").toInt();
    job_options.quality = options.contains("quality") ? options.value("quality").toInt() : 70;
    job_options.fit_to_size = options.value("fitToSize").toBool();
    job_options.target_size_mb = options.value("targetSizeMb").toDouble();
    job_options.audio_bitrate_kbps = options.contains("audioBitrateKbps") ? options.value("audioBitrateKbps").toInt() : 128;
    job_options.export_mode = export_mode;
    job_options.codec = options.value("codec").toString();
    job_options.encoder_backend = QString::fromStdString(m_config.encoder_backend);
    if (job_options.codec.isEmpty())
        job_options.codec = m_config.codec.empty() ? QString("h264") : QString::fromStdString(m_config.codec);

    if (export_mode == "lossless_trim")
    {
        job_options.fit_to_size = false;
        job_options.target_width = 0;
        job_options.target_height = 0;
        job_options.target_fps = 0;
    }
    else if (export_mode == "original_quality")
    {
        job_options.fit_to_size = false;
        job_options.quality = 90;
        if (!options.contains("targetWidth"))
            job_options.target_width = job_options.source_width;
        if (!options.contains("targetHeight"))
            job_options.target_height = job_options.source_height;
        if (!options.contains("targetFps"))
            job_options.target_fps = qRound(metadata ? metadata->fps : options.value("targetFps").toDouble());
    }
    else if (export_mode == "compressed")
    {
        job_options.fit_to_size = false;
        job_options.quality = options.contains("quality") ? options.value("quality").toInt() : 65;
    }
    else if (export_mode == "discord_25mb")
    {
        job_options.fit_to_size = true;
        job_options.target_size_mb = 25.0;
        job_options.quality = 60;
        if (!options.contains("targetWidth"))
            job_options.target_width = qMin(job_options.source_width > 0 ? job_options.source_width : 1280, 1280);
        if (!options.contains("targetHeight"))
            job_options.target_height = qMin(job_options.source_height > 0 ? job_options.source_height : 720, 720);
    }
    else if (export_mode == "twitter_512mb")
    {
        job_options.fit_to_size = true;
        job_options.target_size_mb = 512.0;
        job_options.quality = 75;
    }

    if (job_options.trim_end_sec <= job_options.trim_start_sec)
        job_options.trim_end_sec = job_options.trim_start_sec + 1.0;

    m_export_job = new TranscodeJob(job_options, this);
    connect(m_export_job, &TranscodeJob::progressChanged, this,
            [this, output_path](double progress, const QString& status)
            {
                m_backend->setExportState(true, progress, status, output_path);
            });
    connect(m_export_job, &TranscodeJob::finished, this,
            [this](bool success, const QString& outputPath, const QString& message)
            {
                if (success)
                {
                    const QFileInfo fi(outputPath);
                    m_backend->addRecentClip(outputPath, fi.fileName(),
                        QDateTime::currentDateTime().toString("hh:mm:ss"));
                    m_backend->setExportState(false, 1.0, message, outputPath);
                    if (m_tray)
                        m_tray->showMessage("GhostReplay", message, QSystemTrayIcon::Information, 5000);
                    refreshClipLibrary();
                }
                else
                {
                    m_backend->setExportState(false, 0.0, message);
                    if (message == "Export cancelled.")
                        m_backend->reportInfo(message);
                    else
                        m_backend->reportError(message);
                }

                if (m_export_job)
                {
                    m_export_job->deleteLater();
                    m_export_job = nullptr;
                }
            });
    m_backend->setExportState(true, 0.0, "Preparing export...", output_path);
    m_export_job->start();
}

void GRApplication::onCancelExport()
{
    if (m_export_job)
        m_export_job->cancel();
}

void GRApplication::onOpenClip(const QString& path)
{
    const QFileInfo file_info(path);
    if (!FileActions::isValidLocalFile(path))
    {
        m_backend->reportError("Clip file is missing: " + path);
        refreshClipLibrary();
        return;
    }

    if (!FileActions::launch(FileActions::openFilePlan(path)))
        m_backend->reportError("Failed to open clip: " + file_info.fileName());
}

// ── Slot: Toggle recording ─────────────────────────────────────

void GRApplication::onToggleRecording()
{
    if (!m_capture_available || !m_capture_mgr)
    {
        onRetryCapture();
        return;
    }

    if (m_recording)
    {
        m_capture_mgr->stop();
        if (m_audio_active && m_audio_cap)
            m_audio_cap->stop();
        m_recording = false;
        m_tray->setIcon(appIcon());
        m_action_toggle->setText("Resume Recording");
        LOG_INFO("GRApplication: recording paused");
    }
    else
    {
        if (m_audio_active && m_audio_cap)
        {
            if (m_audio_cap->initialize(m_config.audio))
                m_audio_cap->start();
        }
        m_capture_mgr->start(*m_ring_buffer);
        m_recording = true;
        m_tray->setIcon(appIcon());
        m_action_toggle->setText("Pause Recording");
        LOG_INFO("GRApplication: recording resumed");
    }

    updateTrayTooltip();
    m_backend->setRecording(m_recording);
}

void GRApplication::onSettingsSaved(bool captureRestartRequired)
{
    applySettings(captureRestartRequired);
}

// ── Slot: Open settings window ─────────────────────────────────

void GRApplication::onOpenSettings()
{
    SettingsWindow dlg(m_config);
    Config previous_config = m_config;
    connect(&dlg, &SettingsWindow::settingsApplied, this,
        [this, &previous_config]()
        {
            const bool capture_restart_required =
                previous_config.codec != m_config.codec ||
                previous_config.encoder_backend != m_config.encoder_backend ||
                previous_config.cqp != m_config.cqp ||
                previous_config.preset != m_config.preset ||
                previous_config.fps != m_config.fps ||
                previous_config.capture_width != m_config.capture_width ||
                previous_config.capture_height != m_config.capture_height ||
                previous_config.capture_source != m_config.capture_source ||
                previous_config.keyframe_interval != m_config.keyframe_interval ||
                previous_config.replay_duration_sec != m_config.replay_duration_sec ||
                previous_config.audio.enabled != m_config.audio.enabled ||
                previous_config.audio.channels != m_config.audio.channels ||
                previous_config.audio.sample_rate != m_config.audio.sample_rate ||
                previous_config.audio.bitrate_kbps != m_config.audio.bitrate_kbps;
            applySettings(capture_restart_required);
            previous_config = m_config;
        });
    if (dlg.exec() == QDialog::Accepted)
    {
        m_tray->showMessage("GhostReplay",
            "Settings saved. Changes were applied.",
            QSystemTrayIcon::Information, 4000);
    }
}

// ── Slot: Delete a clip file ─────────────────────────────────

void GRApplication::onDeleteClip(const QString& path)
{
    const QFileInfo file_info(path);
    if (!FileActions::isValidLocalFile(path))
    {
        m_backend->reportError("Clip file is already missing: " + path);
        refreshClipLibrary();
        return;
    }

    if (QFile::remove(path))
    {
        LOG_INFO("Deleted: " + path.toStdString());
        m_backend->reportSuccess("Deleted clip: " + file_info.fileName());
        refreshClipLibrary();
    }
    else
    {
        m_backend->reportError("Failed to delete clip: " + file_info.fileName());
    }
}

// ── Slot: Reveal clip in system file manager ─────────────────

void GRApplication::onRevealInExplorer(const QString& path)
{
    const QFileInfo file_info(path);
    if (!FileActions::isValidLocalFile(path))
    {
        m_backend->reportError("Clip is missing, so it can't be shown in its folder.");
        refreshClipLibrary();
        return;
    }

    if (!FileActions::launch(FileActions::revealFilePlan(path)))
        m_backend->reportError("Failed to show clip in folder: " + file_info.fileName());
}

// ── Slot: Exit ─────────────────────────────────────────────────

void GRApplication::onRetryCapture()
{
    if (m_capture_available)
    {
        m_backend->reportInfo("Capture is already available.");
        return;
    }

    m_backend->reportInfo("Retrying capture startup...");
    if (startCapturePipeline(true))
        m_backend->reportSuccess("Capture is running.");
    else
        m_backend->reportError(m_capture_error.isEmpty() ? "Capture is still unavailable." : m_capture_error);
}

void GRApplication::onOpenLogFile()
{
    const QFileInfo log_info(QString::fromStdString(m_config.log_file));
    if (!log_info.exists())
    {
        m_backend->reportError("Log file has not been created yet.");
        return;
    }

    if (!FileActions::launch(FileActions::openFilePlan(log_info.absoluteFilePath())))
        m_backend->reportError("Failed to open log file.");
}

void GRApplication::onCopyClipToClipboard(const QString& path)
{
    if (!FileActions::isValidLocalFile(path))
    {
        m_backend->reportError("Clip is missing and could not be copied.");
        refreshClipLibrary();
        return;
    }

    if (!FileActions::copyFileToClipboard(path, QApplication::clipboard()))
    {
        m_backend->reportError("Failed to copy clip to clipboard.");
        return;
    }

    m_backend->reportSuccess("Clip copied to clipboard.");
}

void GRApplication::onCopyDiagnostics()
{
    const auto checks = ReleaseHealth::collectStartupChecks(m_config);
    m_health_checks = ReleaseHealth::toVariantList(checks);
    if (m_backend)
        m_backend->setHealthChecks(m_health_checks);

    const QString diagnostics = ReleaseHealth::diagnosticsText(
        m_config, checks, m_capture_available, m_capture_error);
    QApplication::clipboard()->setText(diagnostics);
    m_backend->reportSuccess("Diagnostics copied to clipboard.");
}

void GRApplication::onExit()
{
    shutdown();
    QCoreApplication::quit();
}
