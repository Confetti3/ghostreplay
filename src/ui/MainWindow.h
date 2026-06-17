#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QPixmap>
#include <unordered_map>
#include <memory>
#include <string>

class QTabWidget;
class QListWidget;
class QLabel;
class QPushButton;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QTimer;
class QAction;

class Config;

// ── GhostReplay Main Window ────────────────────────────────────
// Full desktop application with dashboard, clip library, and controls.
// Complements the system-tray icon for users who prefer a windowed UI.

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Config& config, QWidget* parent = nullptr);
    ~MainWindow() override;

    // External events GRApplication forwards to us
    void onCaptureStateChanged(bool recording);
    void onBufferStatsUpdated(int packetCount, int64_t durationUs);
    void onClipSaved(const QString& path, int frameCount, double durationSec);
    void onExportError(const QString& message);
    void onGameChanged(const QString& game);

    // Called by GRApplication to tell us about pipeline objects
    void setPipelineReady(bool ready) { m_pipeline_ready = ready; }

signals:
    void requestSaveClip();
    void requestToggleRecording();
    void requestOpenSettings();
    void requestOpenOutputDir();
    void requestExit();

private slots:
    void onSaveClipClicked();
    void onToggleRecordingClicked();
    void onOpenSettingsClicked();
    void onOpenOutputDirClicked();
    void onRefreshClips();
    void onClipItemDoubleClicked(int row, int column);
    void onClipContextMenu(const QPoint& pos);
    void onUpdateDashboard();
    void onFilterChanged();
    void onGenerateThumbnails();

private:
    void setupUI();
    void setupMenuBar();
    void setupDashboardTab();
    void setupClipsTab();
    void updateRecordingButton(bool recording);
    void refreshClipList();
    void applyFilter();
    QString formatFileSize(qint64 bytes) const;

    Config& m_config;
    bool m_pipeline_ready = false;
    bool m_recording = true;

    // ── Dashboard ──
    QWidget* m_dash_tab = nullptr;
    QLabel* m_status_label = nullptr;
    QLabel* m_buffer_label = nullptr;
    QLabel* m_stats_label = nullptr;
    QPushButton* m_btn_save = nullptr;
    QPushButton* m_btn_toggle = nullptr;
    QPushButton* m_btn_settings = nullptr;
    QPushButton* m_btn_folder = nullptr;
    QListWidget* m_recent_clips = nullptr;

    // ── Clips ──
    QWidget* m_clips_tab = nullptr;
    QLineEdit* m_search_box = nullptr;
    QComboBox* m_game_filter = nullptr;
    QTableWidget* m_clip_table = nullptr;
    QPushButton* m_btn_refresh = nullptr;

    // Thumbnail cache
    std::unordered_map<std::string, QPixmap> m_thumbnail_cache;

    // ── Status bar ──
    QLabel* m_sb_state = nullptr;
    QLabel* m_sb_buffer = nullptr;
    QLabel* m_sb_audio = nullptr;

    // ── Menu ──
    QAction* m_action_save = nullptr;
    QAction* m_action_toggle = nullptr;

    QTimer* m_dash_timer = nullptr;
    int m_recent_clip_count = 0;
};
