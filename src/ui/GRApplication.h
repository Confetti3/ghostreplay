#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

class QWindow;
class CaptureManager;
class RingBuffer;
class AudioCapture;
class HotkeyManager;
class SettingsWindow;
class Backend;
class TranscodeJob;
class DisplayTopologyMonitor;
class Config;
struct EncoderConfig;
struct DisplaySnapshot;

// ── GhostReplay Qt Application ─────────────────────────────────
// Wraps the capture pipeline behind a system-tray icon.
// Exposes a Backend object for QML UI binding.
// Owns all pipeline objects. Thread-safe communication with
// capture thread via signal/slot + mutex-protected buffers.

class GRApplication : public QObject
{
    Q_OBJECT

public:
    explicit GRApplication(Config& config, QObject* parent = nullptr);
    ~GRApplication() override;

    bool initialize();                        // Start capture + audio + hotkey
    void shutdown();                          // Stop everything
    void setBackend(Backend* b) { m_backend = b; }
    void setMainWindow(QWindow* window) { m_main_window = window; }
    bool runTrayRestoreSmokeTest();

private slots:
    void onSaveClip();                        // Hotkey or menu → export
    void onExportClip(const QVariantMap& options);
    void onCancelExport();
    void onSettingsSaved(bool captureRestartRequired);
    void onToggleRecording();                 // Pause / resume capture
    void onOpenSettings();                    // Show settings window
    void onExit();                            // Shutdown from tray menu → quit app
    void onStatusUpdate();                    // Timer → update tray + backend

private:
    void createTrayIcon();
    void connectBackendActions();
    bool startCapturePipeline(bool allowInteractiveWindowPicker);
    void stopCapturePipeline();
    void updateHealthChecks();
    void setCaptureUnavailable(const QString& reason);
    void updateTrayTooltip();
    void rebindHotkey();
    void applySettings(bool captureRestartRequired);
    void initializeDisplayMonitoring();
    void onDisplayTopologySettled();
    void updateBackendDisplayState(const DisplaySnapshot& snapshot);
    void pauseCaptureAfterRestart();
    void restartCaptureAfterDisplayChange(const DisplaySnapshot& snapshot, const QString& reason);
    void showMainWindow();
    void hideMainWindowToTray();
    void performExport();                     // Actual flush + mux
    void refreshClipLibrary();                // Scan output dir → push to Backend
    void onOpenClip(const QString& path);
    void onDeleteClip(const QString& path);
    void onRevealInExplorer(const QString& path);
    void onRetryCapture();
    void onOpenLogFile();
    void onCopyClipToClipboard(const QString& path);
    void onCopyDiagnostics();

    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_tray_menu = nullptr;
    QAction* m_action_status = nullptr;
    QAction* m_action_save = nullptr;
    QAction* m_action_toggle = nullptr;
    QWindow* m_main_window = nullptr;
    DisplayTopologyMonitor* m_display_monitor = nullptr;
    bool m_tray_hint_shown = false;

    Backend* m_backend = nullptr;            // QML data bridge (owned, no parent needed)
    Config& m_config;

    // Pipeline objects
    std::unique_ptr<CaptureManager> m_capture_mgr;
    std::unique_ptr<RingBuffer>     m_ring_buffer;
    std::unique_ptr<AudioCapture>   m_audio_cap;
    std::unique_ptr<HotkeyManager>  m_hotkey_mgr;

    bool m_audio_active = false;
    bool m_recording = true;
    bool m_capture_available = false;
    bool m_backend_connected = false;
    QString m_capture_error;
    QVariantList m_health_checks;

    // Game detection
    std::string m_current_game;
    void pollGameDetection();

    // Encoder config derived from Config
    std::unique_ptr<EncoderConfig> m_enc_cfg;
    TranscodeJob* m_export_job = nullptr;
};
