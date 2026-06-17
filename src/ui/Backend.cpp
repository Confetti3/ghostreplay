#include "ui/Backend.h"
#include "util/Config.h"
#include "util/ReleaseHealth.h"

namespace
{
bool requiresCaptureRestart(const Config& previous, const Config& current)
{
    return previous.codec != current.codec ||
        previous.encoder_backend != current.encoder_backend ||
        previous.cqp != current.cqp ||
        previous.preset != current.preset ||
        previous.fps != current.fps ||
        previous.capture_width != current.capture_width ||
        previous.capture_height != current.capture_height ||
        previous.capture_source != current.capture_source ||
        previous.keyframe_interval != current.keyframe_interval ||
        previous.replay_duration_sec != current.replay_duration_sec ||
        previous.audio.enabled != current.audio.enabled ||
        previous.audio.channels != current.audio.channels ||
        previous.audio.sample_rate != current.audio.sample_rate ||
        previous.audio.bitrate_kbps != current.audio.bitrate_kbps;
}
}

Backend::Backend(QObject* parent) : QObject(parent) {}

// ══ Runtime state accessors ═══════════════════════════════════

bool Backend::isRecording() const { return m_recording; }
void Backend::setRecording(bool v) { if (m_recording != v) { m_recording = v; emit recordingChanged(); } }

int Backend::bufferPackets() const { return m_bufferPackets; }
void Backend::setBufferPackets(int v) { if (m_bufferPackets != v) { m_bufferPackets = v; emit bufferStatsChanged(); } }

int Backend::bufferSeconds() const { return m_bufferSeconds; }
void Backend::setBufferSeconds(int v) { if (m_bufferSeconds != v) { m_bufferSeconds = v; emit bufferStatsChanged(); } }

QString Backend::currentGame() const { return m_currentGame; }
void Backend::setCurrentGame(const QString& v) { if (m_currentGame != v) { m_currentGame = v; emit gameChanged(); } }

bool Backend::isAudioActive() const { return m_audioActive; }
void Backend::setAudioActive(bool v) { if (m_audioActive != v) { m_audioActive = v; emit audioActiveChanged(); } }

QVariantList Backend::recentClips() const { return m_recentClips; }
QVariantList Backend::clipLibrary() const { return m_clipLibrary; }
bool Backend::exportBusy() const { return m_exportBusy; }
double Backend::exportProgress() const { return m_exportProgress; }
QString Backend::exportStatus() const { return m_exportStatus; }
QString Backend::exportOutputPath() const { return m_exportOutputPath; }
bool Backend::captureAvailable() const { return m_captureAvailable; }
QString Backend::captureError() const { return m_captureError; }
QVariantList Backend::healthChecks() const { return m_healthChecks; }
QString Backend::appVersion() const { return ReleaseHealth::appVersion(); }

// ══ Config property accessors ════════════════════════════════

QString Backend::captureCodec() const { return m_captureCodec; }
void Backend::setCaptureCodec(const QString& v) { if (m_captureCodec != v) { m_captureCodec = v; emit captureCodecChanged(); emit configChanged(); } }

QString Backend::encoderBackend() const { return m_encoderBackend; }
void Backend::setEncoderBackend(const QString& v) { if (m_encoderBackend != v) { m_encoderBackend = v; emit encoderBackendChanged(); emit configChanged(); } }

int Backend::captureCqp() const { return m_captureCqp; }
void Backend::setCaptureCqp(int v) { if (m_captureCqp != v) { m_captureCqp = v; emit captureCqpChanged(); emit configChanged(); } }

int Backend::capturePreset() const { return m_capturePreset; }
void Backend::setCapturePreset(int v) { if (m_capturePreset != v) { m_capturePreset = v; emit capturePresetChanged(); emit configChanged(); } }

int Backend::captureFps() const { return m_captureFps; }
void Backend::setCaptureFps(int v) { if (m_captureFps != v) { m_captureFps = v; emit captureFpsChanged(); emit configChanged(); } }

int Backend::captureWidth() const { return m_captureWidth; }
void Backend::setCaptureWidth(int v) { if (m_captureWidth != v) { m_captureWidth = v; emit captureWidthChanged(); emit configChanged(); } }

int Backend::captureHeight() const { return m_captureHeight; }
void Backend::setCaptureHeight(int v) { if (m_captureHeight != v) { m_captureHeight = v; emit captureHeightChanged(); emit configChanged(); } }

QString Backend::captureSource() const { return m_captureSource; }
void Backend::setCaptureSource(const QString& v) { if (m_captureSource != v) { m_captureSource = v; emit captureSourceChanged(); emit configChanged(); } }

int Backend::keyframeInterval() const { return m_keyframeInterval; }
void Backend::setKeyframeInterval(int v) { if (m_keyframeInterval != v) { m_keyframeInterval = v; emit keyframeIntervalChanged(); emit configChanged(); } }

int Backend::replayDurationSec() const { return m_replayDurationSec; }
void Backend::setReplayDurationSec(int v) { if (m_replayDurationSec != v) { m_replayDurationSec = v; emit replayDurationSecChanged(); emit configChanged(); } }

bool Backend::audioEnabled() const { return m_audioEnabled; }
void Backend::setAudioEnabled(bool v) { if (m_audioEnabled != v) { m_audioEnabled = v; emit audioEnabledChanged(); emit configChanged(); } }

int Backend::audioChannels() const { return m_audioChannels; }
void Backend::setAudioChannels(int v) { if (m_audioChannels != v) { m_audioChannels = v; emit audioChannelsChanged(); emit configChanged(); } }

int Backend::audioSampleRate() const { return m_audioSampleRate; }
void Backend::setAudioSampleRate(int v) { if (m_audioSampleRate != v) { m_audioSampleRate = v; emit audioSampleRateChanged(); emit configChanged(); } }

int Backend::audioBitrateKbps() const { return m_audioBitrateKbps; }
void Backend::setAudioBitrateKbps(int v) { if (m_audioBitrateKbps != v) { m_audioBitrateKbps = v; emit audioBitrateKbpsChanged(); emit configChanged(); } }

int Backend::hotkeyModifiers() const { return m_hotkeyModifiers; }
void Backend::setHotkeyModifiers(int v) { if (m_hotkeyModifiers != v) { m_hotkeyModifiers = v; emit hotkeyModifiersChanged(); emit configChanged(); } }

int Backend::hotkeyVk() const { return m_hotkeyVk; }
void Backend::setHotkeyVk(int v) { if (m_hotkeyVk != v) { m_hotkeyVk = v; emit hotkeyVkChanged(); emit configChanged(); } }

QString Backend::outputDir() const { return m_outputDir; }
void Backend::setOutputDir(const QString& v) { if (m_outputDir != v) { m_outputDir = v; emit outputDirChanged(); emit configChanged(); } }

bool Backend::launchAtStartup() const { return m_launchAtStartup; }
void Backend::setLaunchAtStartup(bool v) { if (m_launchAtStartup != v) { m_launchAtStartup = v; emit launchAtStartupChanged(); emit configChanged(); } }

// ══ Sync from Config struct ══════════════════════════════════

void Backend::syncFromConfig()
{
    if (!m_cfg) return;
    m_captureCodec     = QString::fromStdString(m_cfg->codec);
    m_encoderBackend   = QString::fromStdString(m_cfg->encoder_backend);
    m_captureCqp       = m_cfg->cqp;
    m_capturePreset    = m_cfg->preset;
    m_captureFps       = m_cfg->fps;
    m_captureWidth     = m_cfg->capture_width;
    m_captureHeight    = m_cfg->capture_height;
    m_captureSource    = QString::fromStdString(m_cfg->capture_source);
    m_keyframeInterval = m_cfg->keyframe_interval;
    m_replayDurationSec= m_cfg->replay_duration_sec;
    m_audioEnabled     = m_cfg->audio.enabled;
    m_audioChannels    = m_cfg->audio.channels;
    m_audioSampleRate  = m_cfg->audio.sample_rate;
    m_audioBitrateKbps = m_cfg->audio.bitrate_kbps;
    m_hotkeyModifiers  = static_cast<int>(m_cfg->hotkey.modifiers);
    m_hotkeyVk         = static_cast<int>(m_cfg->hotkey.vk);
    m_outputDir        = QString::fromStdString(m_cfg->output_dir);
    m_launchAtStartup  = m_cfg->launch_at_startup;

    // Emit individual signals so QML bindings re-evaluate
    emit captureCodecChanged();
    emit encoderBackendChanged();
    emit captureCqpChanged();
    emit capturePresetChanged();
    emit captureFpsChanged();
    emit captureWidthChanged();
    emit captureHeightChanged();
    emit captureSourceChanged();
    emit keyframeIntervalChanged();
    emit replayDurationSecChanged();
    emit audioEnabledChanged();
    emit audioChannelsChanged();
    emit audioSampleRateChanged();
    emit audioBitrateKbpsChanged();
    emit hotkeyModifiersChanged();
    emit hotkeyVkChanged();
    emit outputDirChanged();
    emit launchAtStartupChanged();
    emit configChanged();
}

// ══ Data push methods ═══════════════════════════════════════

void Backend::addRecentClip(const QString& path, const QString& name, const QString& timestamp)
{
    QVariantMap entry;
    entry["path"] = path;
    entry["name"] = name;
    entry["timestamp"] = timestamp;
    m_recentClips.prepend(entry);
    if (m_recentClips.size() > 10) m_recentClips.removeLast();
    emit recentClipsChanged();
}

void Backend::setClipLibrary(const QVariantList& list)
{
    m_clipLibrary = list;
    emit clipLibraryChanged();
}

void Backend::reportError(const QString& msg)
{
    emit userNotification(msg, "error");
    emit exportError(msg);
}

void Backend::reportInfo(const QString& msg)
{
    emit userNotification(msg, "info");
}

void Backend::reportSuccess(const QString& msg)
{
    emit userNotification(msg, "success");
}

void Backend::setExportState(bool busy, double progress, const QString& status, const QString& outputPath)
{
    bool changed = false;
    if (m_exportBusy != busy)
    {
        m_exportBusy = busy;
        changed = true;
    }
    if (!qFuzzyCompare(m_exportProgress + 1.0, progress + 1.0))
    {
        m_exportProgress = progress;
        changed = true;
    }
    if (m_exportStatus != status)
    {
        m_exportStatus = status;
        changed = true;
    }
    if (!outputPath.isNull() && m_exportOutputPath != outputPath)
    {
        m_exportOutputPath = outputPath;
        changed = true;
    }
    else if (outputPath.isNull() && !busy && !m_exportOutputPath.isEmpty())
    {
        m_exportOutputPath.clear();
        changed = true;
    }
    if (changed)
        emit exportStateChanged();
}

void Backend::setCaptureStatus(bool available, const QString& error)
{
    if (m_captureAvailable == available && m_captureError == error)
        return;

    m_captureAvailable = available;
    m_captureError = error;
    emit captureStatusChanged();
}

void Backend::setHealthChecks(const QVariantList& checks)
{
    m_healthChecks = checks;
    emit healthChecksChanged();
}

// ══ QML user actions ════════════════════════════════════════

void Backend::saveClip()      { emit saveClipRequested(); }
void Backend::exportClip(const QVariantMap& options) { emit exportClipRequested(options); }
void Backend::cancelExport() { emit cancelExportRequested(); }
void Backend::toggleRecording() { emit toggleRecordingRequested(); }
void Backend::openSettings()    { emit openSettingsRequested(); }
void Backend::openOutputDir()   { emit openOutputDirRequested(); }
void Backend::openClip(const QString& path) { emit openClipRequested(path); }

void Backend::deleteClip(const QString& path)
{
    emit deleteClipRequested(path);
}

void Backend::revealInExplorer(const QString& path)
{
    emit revealInExplorerRequested(path);
}

void Backend::hideToTray()
{
    emit hideToTrayRequested();
}

void Backend::requestRefresh()
{
    emit refreshRequested();
}

void Backend::retryCapture()
{
    emit retryCaptureRequested();
}

void Backend::openLogFile()
{
    emit openLogFileRequested();
}

void Backend::copyClipToClipboard(const QString& path)
{
    emit copyClipToClipboardRequested(path);
}

void Backend::copyDiagnostics()
{
    emit copyDiagnosticsRequested();
}

bool Backend::saveConfig()
{
    if (!m_cfg) return false;

    const Config previous_config = *m_cfg;
    m_cfg->codec            = m_captureCodec.toStdString();
    m_cfg->encoder_backend  = m_encoderBackend.toStdString();
    m_cfg->cqp              = m_captureCqp;
    m_cfg->preset           = m_capturePreset;
    m_cfg->fps              = m_captureFps;
    m_cfg->capture_width    = m_captureWidth;
    m_cfg->capture_height   = m_captureHeight;
    m_cfg->capture_source   = m_captureSource.toStdString();
    m_cfg->keyframe_interval= m_keyframeInterval;
    m_cfg->replay_duration_sec = m_replayDurationSec;
    m_cfg->audio.enabled    = m_audioEnabled;
    m_cfg->audio.channels   = m_audioChannels;
    m_cfg->audio.sample_rate= m_audioSampleRate;
    m_cfg->audio.bitrate_kbps = m_audioBitrateKbps;
    m_cfg->hotkey.modifiers = static_cast<unsigned int>(m_hotkeyModifiers);
    m_cfg->hotkey.vk        = static_cast<unsigned int>(m_hotkeyVk);
    m_cfg->output_dir       = m_outputDir.toStdString();
    m_cfg->launch_at_startup = m_launchAtStartup;

    const auto checks = ReleaseHealth::validateConfig(*m_cfg);
    const bool capture_restart_required = requiresCaptureRestart(previous_config, *m_cfg);
    setHealthChecks(ReleaseHealth::toVariantList(checks));
    syncFromConfig();

    bool ok = m_cfg->save("ghostreplay.json");
    if (!ok)
    {
        reportError("Settings could not be saved.");
        return false;
    }

    if (!Config::setStartupEnabled(m_cfg->launch_at_startup))
        reportError("Settings saved, but launch-at-startup could not be updated.");

    emit settingsSaved(capture_restart_required);
    return ok;
}

void Backend::openFolderDialog()
{
    emit openOutputDirRequested();
}
