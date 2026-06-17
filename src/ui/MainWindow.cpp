#include "ui/MainWindow.h"
#include "ui/SettingsWindow.h"
#include "util/Config.h"
#include "util/Logger.h"
#include "util/Thumbnail.h"

#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QKeySequence>
#include <QApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QIcon>
#include <QStyle>
#include <QSet>

// ── Constructor ────────────────────────────────────────────────

MainWindow::MainWindow(Config& config, QWidget* parent)
    : QMainWindow(parent)
    , m_config(config)
{
    setWindowTitle("GhostReplay");
    setMinimumSize(1000, 700);
    resize(1200, 800);

    setupUI();
    setupMenuBar();

    // Dashboard update timer (1 Hz)
    m_dash_timer = new QTimer(this);
    connect(m_dash_timer, &QTimer::timeout, this, &MainWindow::onUpdateDashboard);
    m_dash_timer->start(1000);

    refreshClipList();
}

MainWindow::~MainWindow() = default;

// ── Setup UI ───────────────────────────────────────────────────

void MainWindow::setupUI()
{
    auto* tabs = new QTabWidget(this);

    // ── Dashboard tab ──
    setupDashboardTab();
    tabs->addTab(m_dash_tab, "Dashboard");

    // ── Clips tab ──
    setupClipsTab();
    tabs->addTab(m_clips_tab, "Clips");

    setCentralWidget(tabs);

    // ── Status bar ──
    m_sb_state  = new QLabel("State: Idle");
    m_sb_buffer = new QLabel("Buffer: 0s");
    m_sb_audio  = new QLabel("Audio: —");

    statusBar()->addWidget(m_sb_state);
    statusBar()->addWidget(m_sb_buffer);
    statusBar()->addPermanentWidget(m_sb_audio);

    // ── Tool bar ──
    auto* toolbar = addToolBar("Actions");
    toolbar->setMovable(false);

    m_action_save = toolbar->addAction("Save Clip");
    m_action_save->setShortcut(QKeySequence("Ctrl+S"));
    connect(m_action_save, &QAction::triggered, this, &MainWindow::onSaveClipClicked);

    m_action_toggle = toolbar->addAction("Pause");
    connect(m_action_toggle, &QAction::triggered, this, &MainWindow::onToggleRecordingClicked);

    toolbar->addSeparator();

    auto* action_folder = toolbar->addAction("Open Folder");
    connect(action_folder, &QAction::triggered, this, &MainWindow::onOpenOutputDirClicked);

    auto* action_settings = toolbar->addAction("Settings");
    connect(action_settings, &QAction::triggered, this, &MainWindow::onOpenSettingsClicked);
}

// ── Menu bar ───────────────────────────────────────────────────

void MainWindow::setupMenuBar()
{
    // ── File ──
    QMenu* fileMenu = menuBar()->addMenu("&File");

    auto* saveAct = fileMenu->addAction("&Save Clip", this, &MainWindow::onSaveClipClicked);
    saveAct->setShortcut(QKeySequence::Save);

    fileMenu->addSeparator();

    auto* folderAct = fileMenu->addAction("&Open Clips Folder", this, &MainWindow::onOpenOutputDirClicked);
    folderAct->setShortcut(QKeySequence("Ctrl+O"));

    fileMenu->addSeparator();

    auto* exitAct = fileMenu->addAction("E&xit", this, [this]() { close(); });
    exitAct->setShortcut(QKeySequence::Quit);

    // ── View ──
    QMenu* viewMenu = menuBar()->addMenu("&View");
    auto* refreshAct = viewMenu->addAction("&Refresh Clips", this, &MainWindow::onRefreshClips);
    refreshAct->setShortcut(QKeySequence::Refresh);

    // ── Tools ──
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction("&Settings...", this, &MainWindow::onOpenSettingsClicked);

    // ── Help ──
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this]() {
        QMessageBox::about(this, "About GhostReplay",
            "<h2>GhostReplay</h2>"
            "<p>An instant-replay capture tool for Windows.</p>"
            "<p>Phase 1 build - FFmpeg hardware encoding + AAC + Qt UI</p>");
    });
}

// ── Dashboard tab ──────────────────────────────────────────────

void MainWindow::setupDashboardTab()
{
    m_dash_tab = new QWidget();
    auto* layout = new QVBoxLayout(m_dash_tab);
    layout->setSpacing(20);
    layout->setContentsMargins(20, 20, 20, 20);

    // ── Status group ──
    auto* statusGroup = new QGroupBox("Capture Status");
    auto* statusLayout = new QGridLayout(statusGroup);
    statusLayout->setSpacing(8);

    m_status_label = new QLabel("Recording — capturing screen and audio");
    m_status_label->setProperty("status", "recording");
    statusLayout->addWidget(m_status_label, 0, 0, 1, 2);

    m_buffer_label = new QLabel("Buffer: 0 packets | 0s cached");
    statusLayout->addWidget(m_buffer_label, 1, 0);

    m_stats_label = new QLabel("Clips saved this session: 0");
    statusLayout->addWidget(m_stats_label, 1, 1);

    layout->addWidget(statusGroup);

    // ── Quick actions ──
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(16);

    m_btn_save = new QPushButton("Save Clip (Alt+F10)");
    m_btn_save->setObjectName("btnSave");
    connect(m_btn_save, &QPushButton::clicked, this, &MainWindow::onSaveClipClicked);
    btnLayout->addWidget(m_btn_save);

    m_btn_toggle = new QPushButton("Pause Recording");
    connect(m_btn_toggle, &QPushButton::clicked, this, &MainWindow::onToggleRecordingClicked);
    btnLayout->addWidget(m_btn_toggle);

    m_btn_settings = new QPushButton("Settings...");
    connect(m_btn_settings, &QPushButton::clicked, this, &MainWindow::onOpenSettingsClicked);
    btnLayout->addWidget(m_btn_settings);

    m_btn_folder = new QPushButton("Open Clips Folder");
    connect(m_btn_folder, &QPushButton::clicked, this, &MainWindow::onOpenOutputDirClicked);
    btnLayout->addWidget(m_btn_folder);

    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // ── Recent clips ──
    auto* recentGroup = new QGroupBox("Recent Clips");
    auto* recentLayout = new QVBoxLayout(recentGroup);
    m_recent_clips = new QListWidget();
    m_recent_clips->setMinimumHeight(120);
    connect(m_recent_clips, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    recentLayout->addWidget(m_recent_clips);
    layout->addWidget(recentGroup, 1);
}

// ── Clips tab ──────────────────────────────────────────────────

void MainWindow::setupClipsTab()
{
    m_clips_tab = new QWidget();
    auto* layout = new QVBoxLayout(m_clips_tab);
    layout->setContentsMargins(20, 20, 20, 20);

    // Toolbar with search + filter + refresh
    auto* topLayout = new QHBoxLayout();
    topLayout->setSpacing(8);
    topLayout->setContentsMargins(0, 0, 0, 12);

    m_search_box = new QLineEdit();
    m_search_box->setPlaceholderText("Search clips...");
    m_search_box->setClearButtonEnabled(true);
    connect(m_search_box, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    topLayout->addWidget(m_search_box);
    topLayout->setStretchFactor(m_search_box, 3);

    m_game_filter = new QComboBox();
    m_game_filter->addItem("All Games");
    m_game_filter->setMinimumWidth(140);
    connect(m_game_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);
    topLayout->addWidget(m_game_filter);

    m_btn_refresh = new QPushButton("Refresh");
    connect(m_btn_refresh, &QPushButton::clicked, this, &MainWindow::onRefreshClips);
    topLayout->addWidget(m_btn_refresh);

    layout->addLayout(topLayout);

    // Table with 6 columns: Thumbnail, Name, Game, Date, Size, Duration
    m_clip_table = new QTableWidget(0, 6);
    m_clip_table->setHorizontalHeaderLabels(QStringList() << "" << "Name" << "Game" << "Date" << "Size" << "Duration");
    m_clip_table->horizontalHeader()->setStretchLastSection(false);
    m_clip_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_clip_table->setColumnWidth(0, 140);
    m_clip_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_clip_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_clip_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_clip_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_clip_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_clip_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_clip_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_clip_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_clip_table->setAlternatingRowColors(true);
    m_clip_table->setSortingEnabled(true);
    m_clip_table->verticalHeader()->setDefaultSectionSize(86); // Thumbnail row height
    m_clip_table->setIconSize(QSize(128, 72));
    connect(m_clip_table, &QTableWidget::cellDoubleClicked, this, &MainWindow::onClipItemDoubleClicked);
    connect(m_clip_table, &QTableWidget::customContextMenuRequested, this, &MainWindow::onClipContextMenu);
    layout->addWidget(m_clip_table);
}

// ── External event handlers ────────────────────────────────────

void MainWindow::onCaptureStateChanged(bool recording)
{
    m_recording = recording;
    updateRecordingButton(recording);
}

void MainWindow::onBufferStatsUpdated(int packetCount, int64_t durationUs)
{
    int dur_sec = static_cast<int>(durationUs / 1'000'000);
    m_buffer_label->setText(QString("Buffer: %1 packets | ~%2s cached")
        .arg(packetCount).arg(dur_sec));
    m_sb_buffer->setText(QString("Buffer: %1s").arg(dur_sec));
}

void MainWindow::onClipSaved(const QString& path, int frameCount, double durationSec)
{
    m_recent_clip_count++;
    m_stats_label->setText(QString("Clips saved this session: %1").arg(m_recent_clip_count));

    QFileInfo fi(path);
    QString itemText = QString("[%1] %2 (%3 frames, %4s)")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(fi.fileName())
        .arg(frameCount)
        .arg(durationSec, 0, 'f', 1);

    auto* item = new QListWidgetItem(itemText);
    item->setData(Qt::UserRole, path);
    m_recent_clips->insertItem(0, item);

    // Keep only last 10 in the dashboard list
    while (m_recent_clips->count() > 10)
        delete m_recent_clips->takeItem(m_recent_clips->count() - 1);

    refreshClipList();
}

void MainWindow::onExportError(const QString& message)
{
    QMessageBox::critical(this, "Export Failed", message);
}

void MainWindow::onGameChanged(const QString& game)
{
    m_sb_state->setText(QString("State: %1 | Game: %2")
        .arg(m_recording ? "Recording" : "Paused")
        .arg(game));

    // Add to game filter dropdown if new
    bool found = false;
    for (int i = 0; i < m_game_filter->count(); ++i)
    {
        if (m_game_filter->itemText(i) == game)
        {
            found = true;
            break;
        }
    }
    if (!found && game != "Desktop" && game != "—")
        m_game_filter->addItem(game);
}

// ── Button slots ───────────────────────────────────────────────

void MainWindow::onSaveClipClicked()
{
    emit requestSaveClip();
}

void MainWindow::onToggleRecordingClicked()
{
    emit requestToggleRecording();
}

void MainWindow::onOpenSettingsClicked()
{
    SettingsWindow dlg(m_config, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        QMessageBox::information(this, "Settings Saved",
            "Settings saved. Restart GhostReplay to apply capture changes.");
    }
}

void MainWindow::onOpenOutputDirClicked()
{
    emit requestOpenOutputDir();
}

// ── Clip list management ─────────────────────────────────────

void MainWindow::onRefreshClips()
{
    refreshClipList();
}

void MainWindow::refreshClipList()
{
    // Disable sorting while we repopulate
    m_clip_table->setSortingEnabled(false);

    QDir dir(QString::fromStdString(m_config.output_dir));
    if (!dir.exists())
    {
        m_clip_table->setRowCount(0);
        m_clip_table->setSortingEnabled(true);
        return;
    }

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.mp4", QDir::Files, QDir::Time);

    // Collect unique games for filter dropdown
    QSet<QString> known_games;
    known_games.insert("All Games");
    for (int i = 1; i < m_game_filter->count(); ++i)
        known_games.insert(m_game_filter->itemText(i));

    m_clip_table->setRowCount(0);

    static QRegularExpression re("^(.+?)_(\\d{4}-\\d{2}-\\d{2}_\\d{2}-\\d{2}-\\d{2})\\.mp4$");

    for (const QFileInfo& fi : files)
    {
        int row = m_clip_table->rowCount();
        m_clip_table->insertRow(row);

        QString fname = fi.fileName();

        // Parse game name
        QString game;
        auto match = re.match(fname);
        if (match.hasMatch())
        {
            game = match.captured(1);
            if (game == "GhostReplay") game = "—";
        }
        else
        {
            game = "—";
        }

        // Add game to filter
        if (game != "—" && game != "Desktop")
            known_games.insert(game);

        // ── Column 0: Thumbnail ──
        std::string path_str = fi.absoluteFilePath().toStdString();
        auto it = m_thumbnail_cache.find(path_str);
        if (it != m_thumbnail_cache.end() && !it->second.isNull())
        {
            auto* thumb_item = new QTableWidgetItem();
            thumb_item->setIcon(QIcon(it->second));
            thumb_item->setFlags(thumb_item->flags() & ~Qt::ItemIsEditable);
            m_clip_table->setItem(row, 0, thumb_item);
        }
        else
        {
            auto* thumb_item = new QTableWidgetItem("...");
            thumb_item->setFlags(thumb_item->flags() & ~Qt::ItemIsEditable);
            m_clip_table->setItem(row, 0, thumb_item);
        }

        // ── Columns 1-5: metadata ──
        auto* name_item = new QTableWidgetItem(fname);
        name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);
        m_clip_table->setItem(row, 1, name_item);

        auto* game_item = new QTableWidgetItem(game);
        game_item->setFlags(game_item->flags() & ~Qt::ItemIsEditable);
        m_clip_table->setItem(row, 2, game_item);

        auto* date_item = new QTableWidgetItem(fi.lastModified().toString("yyyy-MM-dd HH:mm"));
        date_item->setFlags(date_item->flags() & ~Qt::ItemIsEditable);
        m_clip_table->setItem(row, 3, date_item);

        auto* size_item = new QTableWidgetItem(formatFileSize(fi.size()));
        size_item->setFlags(size_item->flags() & ~Qt::ItemIsEditable);
        m_clip_table->setItem(row, 4, size_item);

        auto* dur_item = new QTableWidgetItem("—");
        dur_item->setFlags(dur_item->flags() & ~Qt::ItemIsEditable);
        m_clip_table->setItem(row, 5, dur_item);

        // Store full path on all items
        QString abs = fi.absoluteFilePath();
        for (int col = 0; col < 6; ++col)
        {
            if (auto* itm = m_clip_table->item(row, col))
                itm->setData(Qt::UserRole, abs);
        }
    }

    // Rebuild game filter dropdown
    {
        QString selected = m_game_filter->currentText();
        m_game_filter->blockSignals(true);
        m_game_filter->clear();
        QStringList sorted_games(known_games.begin(), known_games.end());
        sorted_games.sort();
        for (const QString& g : sorted_games)
            m_game_filter->addItem(g);
        // Restore previous selection
        int sel_idx = m_game_filter->findText(selected);
        if (sel_idx >= 0)
            m_game_filter->setCurrentIndex(sel_idx);
        m_game_filter->blockSignals(false);
    }

    // Apply search/filter
    applyFilter();
    m_clip_table->setSortingEnabled(true);

    // Start generating thumbnails (scheduled so UI is responsive first)
    QTimer::singleShot(50, this, &MainWindow::onGenerateThumbnails);
}

// ── Search / Filter ──────────────────────────────────────────

void MainWindow::onFilterChanged()
{
    applyFilter();
}

void MainWindow::applyFilter()
{
    QString filter_text = m_search_box ? m_search_box->text().trimmed() : QString();
    QString game_filter = m_game_filter ? m_game_filter->currentText() : "All Games";

    for (int row = 0; row < m_clip_table->rowCount(); ++row)
    {
        bool visible = true;

        // Game filter
        if (game_filter != "All Games")
        {
            auto* game_item = m_clip_table->item(row, 2);
            if (!game_item || game_item->text() != game_filter)
                visible = false;
        }

        // Text filter — matches Name, Game, Date
        if (!filter_text.isEmpty())
        {
            bool matched = false;
            for (int col = 1; col <= 3; ++col)
            {
                auto* itm = m_clip_table->item(row, col);
                if (itm && itm->text().contains(filter_text, Qt::CaseInsensitive))
                {
                    matched = true;
                    break;
                }
            }
            if (!matched) visible = false;
        }

        m_clip_table->setRowHidden(row, !visible);
    }
}

// ── Thumbnail generation ──────────────────────────────────────

void MainWindow::onGenerateThumbnails()
{
    for (int row = 0; row < m_clip_table->rowCount(); ++row)
    {
        if (m_clip_table->isRowHidden(row)) continue;

        QString path = m_clip_table->item(row, 0)->data(Qt::UserRole).toString();
        if (path.isEmpty()) continue;

        std::string path_str = path.toStdString();

        // Skip if already cached
        auto it = m_thumbnail_cache.find(path_str);
        if (it != m_thumbnail_cache.end() && !it->second.isNull())
            continue;

        QPixmap thumb = ThumbnailExtractor::extract(path_str, 128, 72);
        if (!thumb.isNull())
        {
            m_thumbnail_cache[path_str] = thumb;
            m_clip_table->item(row, 0)->setIcon(QIcon(thumb));
            m_clip_table->item(row, 0)->setText("");
        }
        else
        {
            m_clip_table->item(row, 0)->setText("—");
        }

        // Process one row per timer tick to keep UI responsive
        QApplication::processEvents();
    }
}

void MainWindow::onClipItemDoubleClicked(int row, int /*column*/)
{
    QString path = m_clip_table->item(row, 0)->data(Qt::UserRole).toString();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::onClipContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_clip_table->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QString path = m_clip_table->item(row, 0)->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    QMenu menu(this);
    menu.addAction("Open", [path]() { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); });
    menu.addAction("Reveal in Explorer", [path]() {
        QProcess::startDetached("explorer.exe", QStringList() << "/select," << QDir::toNativeSeparators(path));
    });
    menu.addSeparator();
    menu.addAction("Delete", [this, row, path]() {
        auto reply = QMessageBox::question(this, "Delete Clip",
            QString("Delete %1?").arg(QFileInfo(path).fileName()));
        if (reply == QMessageBox::Yes)
        {
            QFile::remove(path);
            refreshClipList();
        }
    });
    menu.exec(m_clip_table->viewport()->mapToGlobal(pos));
}

// ── Helpers ────────────────────────────────────────────────────

void MainWindow::updateRecordingButton(bool recording)
{
    if (recording)
    {
        m_recording = true;
        m_btn_toggle->setText("Pause Recording");
        m_action_toggle->setText("Pause");
        m_status_label->setText("Recording — capturing screen and audio");
        m_status_label->setProperty("status", "recording");
        m_sb_state->setText("State: Recording");
        m_btn_save->setEnabled(true);
        m_action_save->setEnabled(true);
    }
    else
    {
        m_recording = false;
        m_btn_toggle->setText("Resume Recording");
        m_action_toggle->setText("Resume");
        m_status_label->setText("Paused — capture suspended");
        m_status_label->setProperty("status", "paused");
        m_sb_state->setText("State: Paused");
        m_btn_save->setEnabled(false);
        m_action_save->setEnabled(false);
    }

    // Force QSS re-evaluation for dynamic property change
    m_status_label->style()->unpolish(m_status_label);
    m_status_label->style()->polish(m_status_label);
}

void MainWindow::onUpdateDashboard()
{
    // Timer-driven update — mostly stats that are pushed from GRApplication,
    // but we can refresh the clip list periodically here if desired.
    // For now, this is a hook for future live FPS counters.
}

QString MainWindow::formatFileSize(qint64 bytes) const
{
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024 * 1024));
    return QString("%1 GB").arg(bytes / (1024LL * 1024 * 1024));
}
