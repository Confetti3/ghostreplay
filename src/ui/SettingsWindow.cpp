#include "ui/SettingsWindow.h"
#include "util/Config.h"
#include "util/ReleaseHealth.h"
#include "encode/EncoderSelector.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QIntValidator>
#include <QMessageBox>

// ── Constructor ────────────────────────────────────────────────

SettingsWindow::SettingsWindow(Config& config, QWidget* parent)
    : QDialog(parent)
    , m_config(config)
{
    setWindowTitle("GhostReplay Settings");
    setMinimumWidth(520);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(16, 16, 16, 16);

    auto* tabs = new QTabWidget(this);
    createVideoTab(tabs);
    createAudioTab(tabs);
    createBufferTab(tabs);
    createHotkeyTab(tabs);
    createOutputTab(tabs);
    main_layout->addWidget(tabs);

    // Buttons
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsWindow::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsWindow::onApply);
    main_layout->addWidget(buttons);

    configToUi();
}

// ── Video Tab ─────────────────────────────────────────────────

void SettingsWindow::createVideoTab(QTabWidget* tabs)
{
    auto* w = new QWidget();
    auto* form = new QFormLayout(w);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setVerticalSpacing(10);

    m_cmb_capture_source = new QComboBox();
    m_cmb_capture_source->addItem("Desktop", QString("desktop"));
    m_cmb_capture_source->addItem("Window", QString("window"));
    m_cmb_capture_source->addItem("Game", QString("game"));
    m_cmb_capture_source->setToolTip("Window uses the Windows picker. Game captures the foreground game window without hooks.");
    form->addRow("Capture source:", m_cmb_capture_source);

    m_cmb_codec = new QComboBox();
    m_cmb_codec->addItem("H.264", QString("h264"));
    m_cmb_codec->setToolTip("H.264 is the public beta capture codec.");
    form->addRow("Codec:", m_cmb_codec);

    m_cmb_encoder_backend = new QComboBox();
    m_cmb_encoder_backend->addItem("Auto", QString("auto"));
    m_cmb_encoder_backend->addItem("Nvidia NVENC", QString("nvenc"));
    m_cmb_encoder_backend->addItem("AMD AMF", QString("amf"));
    m_cmb_encoder_backend->addItem("Intel Quick Sync", QString("qsv"));
    m_cmb_encoder_backend->addItem("Windows compatibility", QString("mf"));
    m_cmb_encoder_backend->setToolTip("Auto chooses the best encoder for the selected display adapter.");
    form->addRow("Video encoder:", m_cmb_encoder_backend);

    m_spn_cqp = new QSpinBox();
    m_spn_cqp->setRange(0, 51);
    m_spn_cqp->setToolTip("Lower = better quality, higher = smaller file (0–51)");
    form->addRow("Quality (CQP):", m_spn_cqp);

    m_spn_preset = new QSpinBox();
    m_spn_preset->setRange(1, 7);
    m_spn_preset->setToolTip("Encoder preset: 1 = fastest, 7 = best quality");
    form->addRow("Encoder preset:", m_spn_preset);

    m_spn_width = new QSpinBox();
    m_spn_width->setRange(0, 7680);
    m_spn_width->setSpecialValueText("Native");
    m_spn_width->setToolTip("Encoded width. 0 = native capture resolution");
    form->addRow("Width:", m_spn_width);

    m_spn_height = new QSpinBox();
    m_spn_height->setRange(0, 4320);
    m_spn_height->setSpecialValueText("Native");
    m_spn_height->setToolTip("Encoded height. 0 = native capture resolution");
    form->addRow("Height:", m_spn_height);

    m_spn_fps = new QSpinBox();
    m_spn_fps->setRange(1, 240);
    m_spn_fps->setToolTip("Target frame rate");
    form->addRow("FPS:", m_spn_fps);

    m_spn_keyframe = new QSpinBox();
    m_spn_keyframe->setRange(1, 600);
    m_spn_keyframe->setToolTip("IDR keyframe every N frames. Lower = faster seek, larger = smaller file");
    form->addRow("Keyframe interval:", m_spn_keyframe);

    tabs->addTab(w, "Video");
}

// ── Audio Tab ─────────────────────────────────────────────────

void SettingsWindow::createAudioTab(QTabWidget* tabs)
{
    auto* w = new QWidget();
    auto* form = new QFormLayout(w);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setVerticalSpacing(10);

    m_chk_audio_enabled = new QCheckBox();
    form->addRow("Audio enabled:", m_chk_audio_enabled);

    m_spn_audio_channels = new QSpinBox();
    m_spn_audio_channels->setRange(1, 2);
    form->addRow("Channels:", m_spn_audio_channels);

    m_cmb_audio_sample_rate = new QComboBox();
    m_cmb_audio_sample_rate->addItem("44100 Hz", 44100);
    m_cmb_audio_sample_rate->addItem("48000 Hz", 48000);
    m_cmb_audio_sample_rate->addItem("96000 Hz", 96000);
    form->addRow("Sample rate:", m_cmb_audio_sample_rate);

    m_spn_audio_bitrate = new QSpinBox();
    m_spn_audio_bitrate->setRange(64, 320);
    m_spn_audio_bitrate->setSuffix(" kbps");
    m_spn_audio_bitrate->setSingleStep(32);
    form->addRow("AAC bitrate:", m_spn_audio_bitrate);

    tabs->addTab(w, "Audio");
}

// ── Buffer Tab ────────────────────────────────────────────────

void SettingsWindow::createBufferTab(QTabWidget* tabs)
{
    auto* w = new QWidget();
    auto* form = new QFormLayout(w);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setVerticalSpacing(10);

    m_spn_duration = new QSpinBox();
    m_spn_duration->setRange(15, 3600);
    m_spn_duration->setSuffix(" seconds");
    m_spn_duration->setSingleStep(30);
    m_spn_duration->setToolTip("How far back the replay buffer remembers. Longer = more RAM usage.");
    form->addRow("Replay duration:", m_spn_duration);

    tabs->addTab(w, "Buffer");
}

// ── Hotkey Tab ────────────────────────────────────────────────

void SettingsWindow::createHotkeyTab(QTabWidget* tabs)
{
    auto* w = new QWidget();
    auto* form = new QFormLayout(w);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setVerticalSpacing(10);

    m_cmb_modifier = new QComboBox();
    m_cmb_modifier->addItem("Alt",      0x0001);   // MOD_ALT
    m_cmb_modifier->addItem("Ctrl",     0x0002);   // MOD_CONTROL
    m_cmb_modifier->addItem("Shift",    0x0004);   // MOD_SHIFT
    m_cmb_modifier->addItem("Win",      0x0008);   // MOD_WIN
    m_cmb_modifier->addItem("Alt+Ctrl", 0x0003);
    m_cmb_modifier->addItem("Alt+Shift",0x0005);
    m_cmb_modifier->addItem("Ctrl+Shift",0x0006);
    m_cmb_modifier->setToolTip("Modifier key(s) for the save-clip hotkey");
    form->addRow("Modifier:", m_cmb_modifier);

    m_spn_vk = new QSpinBox();
    m_spn_vk->setRange(0x01, 0xFF);
    m_spn_vk->setDisplayIntegerBase(16);
    m_spn_vk->setPrefix("0x");
    m_spn_vk->setToolTip("Virtual key code (hex). Common: F10=0x79, F9=0x78, F8=0x77");
    form->addRow("Key (VK):", m_spn_vk);

    tabs->addTab(w, "Hotkey");
}

// ── Output Tab ────────────────────────────────────────────────

void SettingsWindow::createOutputTab(QTabWidget* tabs)
{
    auto* w = new QWidget();
    auto* form = new QFormLayout(w);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setVerticalSpacing(10);

    auto* dir_layout = new QHBoxLayout();
    dir_layout->setSpacing(8);
    m_edt_output_dir = new QLineEdit();
    dir_layout->addWidget(m_edt_output_dir);

    auto* browse_btn = new QPushButton("Browse...");
    connect(browse_btn, &QPushButton::clicked, [this]()
    {
        QString dir = QFileDialog::getExistingDirectory(this,
            "Select output directory", m_edt_output_dir->text());
        if (!dir.isEmpty())
            m_edt_output_dir->setText(dir);
    });
    dir_layout->addWidget(browse_btn);

    form->addRow("Output directory:", dir_layout);

    m_chk_startup = new QCheckBox("Launch GhostReplay when Windows starts");
    form->addRow(m_chk_startup);

    tabs->addTab(w, "Output");
}

// ── Sync config → UI ──────────────────────────────────────────

void SettingsWindow::configToUi()
{
    // Video
    int source_idx = m_cmb_capture_source->findData(QString::fromStdString(m_config.capture_source));
    if (source_idx >= 0) m_cmb_capture_source->setCurrentIndex(source_idx);
    int codec_idx = m_cmb_codec->findData(QString::fromStdString(m_config.codec));
    if (codec_idx >= 0) m_cmb_codec->setCurrentIndex(codec_idx);
    int backend_idx = m_cmb_encoder_backend->findData(
        QString::fromStdString(encoderBackendToString(encoderBackendFromString(m_config.encoder_backend))));
    if (backend_idx >= 0) m_cmb_encoder_backend->setCurrentIndex(backend_idx);
    m_spn_cqp->setValue(m_config.cqp);
    m_spn_preset->setValue(m_config.preset);
    m_spn_width->setValue(m_config.capture_width);
    m_spn_height->setValue(m_config.capture_height);
    m_spn_fps->setValue(m_config.fps);
    m_spn_keyframe->setValue(m_config.keyframe_interval);

    // Audio
    m_chk_audio_enabled->setChecked(m_config.audio.enabled);
    m_spn_audio_channels->setValue(m_config.audio.channels);
    int sr_idx = m_cmb_audio_sample_rate->findData(m_config.audio.sample_rate);
    if (sr_idx >= 0) m_cmb_audio_sample_rate->setCurrentIndex(sr_idx);
    m_spn_audio_bitrate->setValue(m_config.audio.bitrate_kbps);

    // Buffer
    m_spn_duration->setValue(m_config.replay_duration_sec);

    // Hotkey
    int mod_idx = m_cmb_modifier->findData(static_cast<int>(m_config.hotkey.modifiers));
    if (mod_idx >= 0) m_cmb_modifier->setCurrentIndex(mod_idx);
    m_spn_vk->setValue(static_cast<int>(m_config.hotkey.vk));

    // Output
    m_edt_output_dir->setText(QString::fromStdString(m_config.output_dir));
    m_chk_startup->setChecked(m_config.launch_at_startup);
}

// ── Sync UI → config ──────────────────────────────────────────

void SettingsWindow::uiToConfig()
{
    // Video
    m_config.capture_source = m_cmb_capture_source->currentData().toString().toStdString();
    m_config.codec = m_cmb_codec->currentData().toString().toStdString();
    m_config.encoder_backend = m_cmb_encoder_backend->currentData().toString().toStdString();
    m_config.cqp = m_spn_cqp->value();
    m_config.preset = m_spn_preset->value();
    m_config.capture_width = m_spn_width->value();
    m_config.capture_height = m_spn_height->value();
    m_config.fps = m_spn_fps->value();
    m_config.keyframe_interval = m_spn_keyframe->value();

    // Audio
    m_config.audio.enabled = m_chk_audio_enabled->isChecked();
    m_config.audio.channels = m_spn_audio_channels->value();
    m_config.audio.sample_rate = m_cmb_audio_sample_rate->currentData().toInt();
    m_config.audio.bitrate_kbps = m_spn_audio_bitrate->value();

    // Buffer
    m_config.replay_duration_sec = m_spn_duration->value();

    // Hotkey
    m_config.hotkey.modifiers = static_cast<unsigned int>(m_cmb_modifier->currentData().toInt());
    m_config.hotkey.vk = static_cast<unsigned int>(m_spn_vk->value());

    // Output
    m_config.output_dir = m_edt_output_dir->text().toStdString();
    m_config.launch_at_startup = m_chk_startup->isChecked();
}

// ── Slots ─────────────────────────────────────────────────────

void SettingsWindow::onApply()
{
    if (applyChanges())
        emit settingsApplied();
}

void SettingsWindow::onAccepted()
{
    if (applyChanges())
    {
        emit settingsApplied();
        accept();
    }
}

bool SettingsWindow::applyChanges()
{
    uiToConfig();
    ReleaseHealth::validateConfig(m_config);
    configToUi();

    if (!m_config.save("ghostreplay.json"))
    {
        QMessageBox::warning(this, "Settings Not Saved",
            "Ghost Replay could not write ghostreplay.json. Choose a writable folder or check file permissions.");
        return false;
    }

    if (!Config::setStartupEnabled(m_config.launch_at_startup))
    {
        QMessageBox::warning(this, "Startup Setting Not Updated",
            "Settings were saved, but the Windows startup setting could not be updated.");
    }

    return true;
}
