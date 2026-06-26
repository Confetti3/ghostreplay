#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>

class Config;

// Thin bridge between QML UI and the capture pipeline.
// GRApplication updates the properties; QML reads them.
// QML actions fire invokable methods → signals → GRApplication handles.
// Also exposes Config settings for the QML Settings flyout.

class Backend : public QObject
{
    Q_OBJECT
    // ── Runtime state ───────────────────────────────────────
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)
    Q_PROPERTY(int bufferPackets READ bufferPackets WRITE setBufferPackets NOTIFY bufferStatsChanged)
    Q_PROPERTY(int bufferSeconds READ bufferSeconds WRITE setBufferSeconds NOTIFY bufferStatsChanged)
    Q_PROPERTY(QString currentGame READ currentGame WRITE setCurrentGame NOTIFY gameChanged)
    Q_PROPERTY(bool audioActive READ isAudioActive WRITE setAudioActive NOTIFY audioActiveChanged)
    Q_PROPERTY(QVariantList recentClips READ recentClips NOTIFY recentClipsChanged)
    Q_PROPERTY(QVariantList clipLibrary READ clipLibrary NOTIFY clipLibraryChanged)
    Q_PROPERTY(bool exportBusy READ exportBusy NOTIFY exportStateChanged)
    Q_PROPERTY(double exportProgress READ exportProgress NOTIFY exportStateChanged)
    Q_PROPERTY(QString exportStatus READ exportStatus NOTIFY exportStateChanged)
    Q_PROPERTY(QString exportOutputPath READ exportOutputPath NOTIFY exportStateChanged)
    Q_PROPERTY(bool captureAvailable READ captureAvailable NOTIFY captureStatusChanged)
    Q_PROPERTY(QString captureError READ captureError NOTIFY captureStatusChanged)
    Q_PROPERTY(QVariantList healthChecks READ healthChecks NOTIFY healthChecksChanged)
    Q_PROPERTY(QString captureDisplayLabel READ captureDisplayLabel NOTIFY displayStateChanged)
    Q_PROPERTY(QString captureDisplayResolution READ captureDisplayResolution NOTIFY displayStateChanged)
    Q_PROPERTY(QString captureDisplayScale READ captureDisplayScale NOTIFY displayStateChanged)
    Q_PROPERTY(bool autoDisplayAdaptationEnabled READ autoDisplayAdaptationEnabled NOTIFY displayStateChanged)
    Q_PROPERTY(QVariantList availableDisplays READ availableDisplays NOTIFY displayStateChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)

    // ── Config settings (QML flyout reads/writes these) ────
    Q_PROPERTY(QString captureCodec READ captureCodec WRITE setCaptureCodec NOTIFY captureCodecChanged)
    Q_PROPERTY(QString encoderBackend READ encoderBackend WRITE setEncoderBackend NOTIFY encoderBackendChanged)
    Q_PROPERTY(int captureCqp READ captureCqp WRITE setCaptureCqp NOTIFY captureCqpChanged)
    Q_PROPERTY(int capturePreset READ capturePreset WRITE setCapturePreset NOTIFY capturePresetChanged)
    Q_PROPERTY(int captureFps READ captureFps WRITE setCaptureFps NOTIFY captureFpsChanged)
    Q_PROPERTY(int captureWidth READ captureWidth WRITE setCaptureWidth NOTIFY captureWidthChanged)
    Q_PROPERTY(int captureHeight READ captureHeight WRITE setCaptureHeight NOTIFY captureHeightChanged)
    Q_PROPERTY(QString captureSource READ captureSource WRITE setCaptureSource NOTIFY captureSourceChanged)
    Q_PROPERTY(int keyframeInterval READ keyframeInterval WRITE setKeyframeInterval NOTIFY keyframeIntervalChanged)
    Q_PROPERTY(int replayDurationSec READ replayDurationSec WRITE setReplayDurationSec NOTIFY replayDurationSecChanged)
    Q_PROPERTY(bool audioEnabled READ audioEnabled WRITE setAudioEnabled NOTIFY audioEnabledChanged)
    Q_PROPERTY(int audioChannels READ audioChannels WRITE setAudioChannels NOTIFY audioChannelsChanged)
    Q_PROPERTY(int audioSampleRate READ audioSampleRate WRITE setAudioSampleRate NOTIFY audioSampleRateChanged)
    Q_PROPERTY(int audioBitrateKbps READ audioBitrateKbps WRITE setAudioBitrateKbps NOTIFY audioBitrateKbpsChanged)
    Q_PROPERTY(int hotkeyModifiers READ hotkeyModifiers WRITE setHotkeyModifiers NOTIFY hotkeyModifiersChanged)
    Q_PROPERTY(int hotkeyVk READ hotkeyVk WRITE setHotkeyVk NOTIFY hotkeyVkChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool launchAtStartup READ launchAtStartup WRITE setLaunchAtStartup NOTIFY launchAtStartupChanged)
    Q_PROPERTY(int monitorIndex READ monitorIndex WRITE setMonitorIndex NOTIFY monitorIndexChanged)

public:
    explicit Backend(QObject* parent = nullptr);

    void setConfig(Config* cfg) { m_cfg = cfg; syncFromConfig(); }

    // ── Property accessors ────────────────────────────────
    bool isRecording() const;
    void setRecording(bool v);
    int bufferPackets() const;
    void setBufferPackets(int v);
    int bufferSeconds() const;
    void setBufferSeconds(int v);
    QString currentGame() const;
    void setCurrentGame(const QString& v);
    bool isAudioActive() const;
    void setAudioActive(bool v);
    QVariantList recentClips() const;
    QVariantList clipLibrary() const;
    bool exportBusy() const;
    double exportProgress() const;
    QString exportStatus() const;
    QString exportOutputPath() const;
    bool captureAvailable() const;
    QString captureError() const;
    QVariantList healthChecks() const;
    QString captureDisplayLabel() const;
    QString captureDisplayResolution() const;
    QString captureDisplayScale() const;
    bool autoDisplayAdaptationEnabled() const;
    QVariantList availableDisplays() const;
    QString appVersion() const;

    // Config accessors
    QString captureCodec() const;
    void setCaptureCodec(const QString& v);
    QString encoderBackend() const;
    void setEncoderBackend(const QString& v);
    int captureCqp() const;
    void setCaptureCqp(int v);
    int capturePreset() const;
    void setCapturePreset(int v);
    int captureFps() const;
    void setCaptureFps(int v);
    int captureWidth() const;
    void setCaptureWidth(int v);
    int captureHeight() const;
    void setCaptureHeight(int v);
    QString captureSource() const;
    void setCaptureSource(const QString& v);
    int keyframeInterval() const;
    void setKeyframeInterval(int v);
    int replayDurationSec() const;
    void setReplayDurationSec(int v);
    bool audioEnabled() const;
    void setAudioEnabled(bool v);
    int audioChannels() const;
    void setAudioChannels(int v);
    int audioSampleRate() const;
    void setAudioSampleRate(int v);
    int audioBitrateKbps() const;
    void setAudioBitrateKbps(int v);
    int hotkeyModifiers() const;
    void setHotkeyModifiers(int v);
    int hotkeyVk() const;
    void setHotkeyVk(int v);
    QString outputDir() const;
    void setOutputDir(const QString& v);
    bool launchAtStartup() const;
    void setLaunchAtStartup(bool v);
    int monitorIndex() const;
    void setMonitorIndex(int v);

    // ── Data push (called by GRApplication) ───────────────
    void addRecentClip(const QString& path, const QString& name, const QString& timestamp);
    void setClipLibrary(const QVariantList& list);
    void reportError(const QString& msg);
    void reportInfo(const QString& msg);
    void reportSuccess(const QString& msg);
    void setExportState(bool busy, double progress, const QString& status, const QString& outputPath = QString());
    void setCaptureStatus(bool available, const QString& error);
    void setHealthChecks(const QVariantList& checks);
    void setDisplayState(const QString& label,
                         const QString& resolution,
                         const QString& scale,
                         bool autoAdaptationEnabled);

    // ── QML user actions ──────────────────────────────────
    Q_INVOKABLE void saveClip();
    Q_INVOKABLE void exportClip(const QVariantMap& options);
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE void toggleRecording();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void openOutputDir();
    Q_INVOKABLE void openClip(const QString& path);
    Q_INVOKABLE void deleteClip(const QString& path);
    Q_INVOKABLE void revealInExplorer(const QString& path);
    Q_INVOKABLE void hideToTray();
    Q_INVOKABLE void requestRefresh();
    Q_INVOKABLE void retryCapture();
    Q_INVOKABLE void openLogFile();
    Q_INVOKABLE void copyClipToClipboard(const QString& path);
    Q_INVOKABLE void copyDiagnostics();
    Q_INVOKABLE bool saveConfig();          // Writes settings to ghostreplay.json
    Q_INVOKABLE void openFolderDialog();    // Native folder picker → outputDir

signals:
    void recordingChanged();
    void bufferStatsChanged();
    void gameChanged();
    void audioActiveChanged();
    void recentClipsChanged();
    void clipLibraryChanged();
    void exportStateChanged();
    void captureStatusChanged();
    void healthChecksChanged();
    void displayStateChanged();
    void configChanged();                    // Generic config change

    // Individual config property signals for QML binding
    void captureCodecChanged();
    void encoderBackendChanged();
    void captureCqpChanged();
    void capturePresetChanged();
    void captureFpsChanged();
    void captureWidthChanged();
    void captureHeightChanged();
    void captureSourceChanged();
    void keyframeIntervalChanged();
    void replayDurationSecChanged();
    void audioEnabledChanged();
    void audioChannelsChanged();
    void audioSampleRateChanged();
    void audioBitrateKbpsChanged();
    void hotkeyModifiersChanged();
    void hotkeyVkChanged();
    void outputDirChanged();
    void launchAtStartupChanged();
    void monitorIndexChanged();

    void settingsSaved(bool restartRequired);
    void userNotification(const QString& msg, const QString& severity);

    // Action signals (GRApplication connects to these)
    void saveClipRequested();
    void exportClipRequested(const QVariantMap& options);
    void cancelExportRequested();
    void toggleRecordingRequested();
    void openSettingsRequested();
    void openOutputDirRequested();
    void openClipRequested(const QString& path);
    void deleteClipRequested(const QString& path);
    void revealInExplorerRequested(const QString& path);
    void hideToTrayRequested();
    void refreshRequested();
    void retryCaptureRequested();
    void openLogFileRequested();
    void copyClipToClipboardRequested(const QString& path);
    void copyDiagnosticsRequested();
    void exportError(const QString& msg);

private:
    void syncFromConfig();   // Push m_cfg values into Q_PROPERTY fields

    bool m_recording = true;
    int m_bufferPackets = 0;
    int m_bufferSeconds = 0;
    QString m_currentGame = "Desktop";
    bool m_audioActive = false;
    QVariantList m_recentClips;
    QVariantList m_clipLibrary;
    bool m_exportBusy = false;
    double m_exportProgress = 0.0;
    QString m_exportStatus;
    QString m_exportOutputPath;
    bool m_captureAvailable = false;
    QString m_captureError;
    QVariantList m_healthChecks;
    QString m_captureDisplayLabel = "Display";
    QString m_captureDisplayResolution = "Native";
    QString m_captureDisplayScale = "100%";
    bool m_autoDisplayAdaptationEnabled = true;

    Config* m_cfg = nullptr;

    // Cached config values (Q_PROPERTY backing)
    QString m_captureCodec = "h264";
    QString m_encoderBackend = "auto";
    int m_captureCqp = 23;
    int m_capturePreset = 4;
    int m_captureFps = 60;
    int m_captureWidth = 0;
    int m_captureHeight = 0;
    QString m_captureSource = "desktop";
    int m_keyframeInterval = 120;
    int m_replayDurationSec = 300;
    bool m_audioEnabled = true;
    int m_audioChannels = 2;
    int m_audioSampleRate = 48000;
    int m_audioBitrateKbps = 192;
    int m_hotkeyModifiers = 1;
    int m_hotkeyVk = 121;
    QString m_outputDir = "clips";
    bool m_launchAtStartup = false;
    int m_monitorIndex = 0;
};
