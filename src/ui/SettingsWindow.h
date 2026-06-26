#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>

class Config;

// ── GhostReplay Settings Window ─────────────────────────────────
// Tabbed dialog for editing ghostreplay.json settings.
// Saves config on Accept (OK button).

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(Config& config, QWidget* parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void onApply();
    void onAccepted();

private:
    void createVideoTab(QTabWidget* tabs);
    void createAudioTab(QTabWidget* tabs);
    void createBufferTab(QTabWidget* tabs);
    void createHotkeyTab(QTabWidget* tabs);
    void createOutputTab(QTabWidget* tabs);

    void configToUi();
    void uiToConfig();
    bool applyChanges();

    Config& m_config;

    // Video tab widgets
    QComboBox* m_cmb_capture_source = nullptr;
    QComboBox* m_cmb_monitor_index = nullptr;
    QComboBox* m_cmb_codec = nullptr;
    QComboBox* m_cmb_encoder_backend = nullptr;
    QSpinBox*  m_spn_cqp = nullptr;
    QSpinBox*  m_spn_preset = nullptr;
    QSpinBox*  m_spn_width = nullptr;
    QSpinBox*  m_spn_height = nullptr;
    QSpinBox*  m_spn_fps = nullptr;
    QSpinBox*  m_spn_keyframe = nullptr;

    // Audio tab widgets
    QCheckBox* m_chk_audio_enabled = nullptr;
    QSpinBox*  m_spn_audio_channels = nullptr;
    QComboBox* m_cmb_audio_sample_rate = nullptr;
    QSpinBox*  m_spn_audio_bitrate = nullptr;

    // Buffer tab
    QSpinBox*  m_spn_duration = nullptr;

    // Hotkey tab
    QComboBox* m_cmb_modifier = nullptr;
    QSpinBox*  m_spn_vk = nullptr;

    // Output tab
    QLineEdit* m_edt_output_dir = nullptr;
    QCheckBox* m_chk_startup = nullptr;
};
