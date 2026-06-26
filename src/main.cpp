// ── GhostReplay Phase 1 ───────────────────────────────────────
// Background replay capture: DXGI -> FFmpeg hardware encoder -> ring buffer -> MP4.
// Phase 1 adds WASAPI audio, JSON config, and Qt system tray.
//
// When HAS_QT6 is defined, runs as a system-tray application.
// Without Qt, falls back to the console interface.
// ──────────────────────────────────────────────────────────────

#include "util/Config.h"
#include "util/Logger.h"

#include <Windows.h>   // CreateDirectoryA
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

#ifdef HAS_QT6
#  include "ui/GRApplication.h"
#  include "ui/Backend.h"
#  include "ui/ClipPreviewController.h"
#  include "ui/ThumbnailProvider.h"
#  include "ui/WindowChromeController.h"
#  include <QAbstractNativeEventFilter>
#  include <QApplication>
#  include <QGuiApplication>
#  include <QIcon>
#  include <QQmlApplicationEngine>
#  include <QQmlContext>
#  include <QQmlError>
#  include <QQuickStyle>
#  include <QQuickWindow>
#  include <QScreen>
#  include <QTimer>
#  include <QWindow>
#  include <QtGlobal>
#  include <QLocalServer>
#  include <QLocalSocket>
#  include <algorithm>
#  include <cstring>
#  include <iostream>
#  include <memory>
#  ifdef Q_OS_WIN
#    include <dwmapi.h>
#  endif
#else
#  include "capture/CaptureManager.h"
#  include "buffer/RingBuffer.h"
#  include "hotkey/HotkeyManager.h"
#  include "export/ClipExporter.h"
#  include "audio/AudioCapture.h"
#  include "encode/EncoderConfig.h"
#  include "encode/EncoderSelector.h"
#  include <Windows.h>
#  include <iostream>
#  include <csignal>
#  include <atomic>
#  include <thread>
#  include <chrono>
#  include <cstring>
   using namespace std::chrono_literals;
#endif

// ── Entry point ───────────────────────────────────────────────

namespace
{
Logger::Level loggerLevelFromConfig(int level)
{
    return static_cast<Logger::Level>(std::clamp(level, 0, 3));
}

void ensureOutputDirectory(const std::string& outputDir)
{
    if (outputDir.empty())
        return;

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec)
        LOG_WARNING("main: could not create output directory '" + outputDir + "': " + ec.message());
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

#ifdef HAS_QT6
void fitWindowToAvailableScreen(QWindow* window, QApplication& app, bool recenter)
{
    if (!window)
        return;

    QScreen* screen = window->screen();
    if (!screen)
        screen = app.primaryScreen();
    if (!screen)
        return;

    const QRect available = screen->availableGeometry();
    if (!available.isValid())
        return;

    const int min_width = std::min(860, std::max(640, available.width()));
    const int min_height = std::min(600, std::max(520, available.height()));
    window->setMinimumSize(QSize(min_width, min_height));

    if ((window->windowStates() & Qt::WindowMaximized) || window->visibility() == QWindow::Maximized)
        return;

    const int desired_width = 1268;
    const int desired_height = 720;
    const int current_width = window->width() > 0 ? window->width() : desired_width;
    const int current_height = window->height() > 0 ? window->height() : desired_height;
    const int width = std::min(std::max(min_width, current_width), available.width());
    const int height = std::min(std::max(min_height, current_height), available.height());

    QRect nextGeometry(window->position(), QSize(width, height));
    if (recenter || !available.contains(nextGeometry))
    {
        const int x = available.x() + std::max(0, (available.width() - width) / 2);
        const int y = available.y() + std::max(0, (available.height() - height) / 2);
        nextGeometry.moveTopLeft(QPoint(x, y));
    }

    if (window->size() != nextGeometry.size())
        window->resize(nextGeometry.size());
    if (window->position() != nextGeometry.topLeft())
        window->setPosition(nextGeometry.topLeft());
}

void wireWindowScreenClamping(QWindow* window, QApplication& app)
{
    if (!window)
        return;

    auto clampLater = [window, &app](bool recenter)
    {
        QTimer::singleShot(0, window, [window, &app, recenter]()
        {
            fitWindowToAvailableScreen(window, app, recenter);
        });
    };

    QObject::connect(window, &QWindow::screenChanged, window,
        [window, &app, clampLater](QScreen* screen)
        {
            if (screen)
            {
                QObject::connect(screen, &QScreen::availableGeometryChanged, window,
                    [clampLater]() { clampLater(false); }, Qt::UniqueConnection);
                QObject::connect(screen, &QScreen::geometryChanged, window,
                    [clampLater]() { clampLater(false); }, Qt::UniqueConnection);
                QObject::connect(screen, &QScreen::logicalDotsPerInchChanged, window,
                    [clampLater]() { clampLater(false); }, Qt::UniqueConnection);
            }
            clampLater(false);
        });

    QObject::connect(&app, &QGuiApplication::screenAdded, window,
        [clampLater](QScreen*) { clampLater(false); });
    QObject::connect(&app, &QGuiApplication::screenRemoved, window,
        [clampLater](QScreen*) { clampLater(true); });

    if (QScreen* screen = window->screen())
    {
        QObject::connect(screen, &QScreen::availableGeometryChanged, window,
            [clampLater]() { clampLater(false); }, Qt::UniqueConnection);
        QObject::connect(screen, &QScreen::geometryChanged, window,
            [clampLater]() { clampLater(false); }, Qt::UniqueConnection);
        QObject::connect(screen, &QScreen::logicalDotsPerInchChanged, window,
            [clampLater]() { clampLater(false); }, Qt::UniqueConnection);
    }
}

class ShellDpiEventFilter final : public QAbstractNativeEventFilter
{
public:
    ShellDpiEventFilter(QWindow* window, QApplication& app)
        : m_window(window)
        , m_app(app)
    {
    }

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
    {
        Q_UNUSED(result);

#ifdef Q_OS_WIN
        if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
            return false;

        const MSG* msg = static_cast<MSG*>(message);
        if (!msg || !m_window)
            return false;

        const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        if (!hwnd || msg->hwnd != hwnd)
            return false;

        switch (msg->message)
        {
        case WM_DPICHANGED:
            preserveWindowMetrics();
            scheduleClamp(false);
            break;
        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
            preserveWindowMetrics();
            scheduleClamp(false);
            break;
        default:
            break;
        }
#else
        Q_UNUSED(eventType);
        Q_UNUSED(message);
#endif

        return false;
    }

private:
    double currentWindowScale() const
    {
        QScreen* screen = m_window ? m_window->screen() : nullptr;
        if (!screen)
            screen = m_app.primaryScreen();
        if (!screen)
            return 1.0;

        const qreal dpi = screen->logicalDotsPerInch();
        if (dpi <= 0.0)
            return 1.0;

        return std::max(0.5, static_cast<double>(dpi / 96.0));
    }

    bool isWindowMaximized() const
    {
        return m_window
            && (((m_window->windowStates() & Qt::WindowMaximized) != 0)
                || m_window->visibility() == QWindow::Maximized);
    }

    void preserveWindowMetrics()
    {
        if (!m_window || m_restorePending || isWindowMaximized())
            return;

        const QSize currentSize = m_window->size();
        if (!currentSize.isValid() || currentSize.isEmpty())
            return;

        m_preservedLogicalSize = currentSize;
        m_preservedScale = currentWindowScale();
        m_restorePending = true;
    }

    void normalizeWindowAfterDpiChange(bool recenter, bool finalize)
    {
        if (!m_window)
            return;

        if (m_restorePending && !isWindowMaximized() && m_preservedLogicalSize.isValid())
        {
            const QSize targetLogicalSize = m_preservedLogicalSize;

            if (m_window->size() != targetLogicalSize)
                m_window->resize(targetLogicalSize);
        }

        fitWindowToAvailableScreen(m_window, m_app, recenter);

        if (finalize)
            m_restorePending = false;
    }

    void scheduleClamp(bool recenter)
    {
        if (!m_window)
            return;

        QTimer::singleShot(0, m_window, [this, recenter]()
        {
            normalizeWindowAfterDpiChange(recenter, false);
        });
        QTimer::singleShot(150, m_window, [this, recenter]()
        {
            normalizeWindowAfterDpiChange(recenter, true);
        });
    }

    QWindow* m_window = nullptr;
    QApplication& m_app;
    QSize m_preservedLogicalSize;
    double m_preservedScale = 1.0;
    bool m_restorePending = false;
};

#ifdef Q_OS_WIN
void applyWindowsRoundedCorners(QWindow* window)
{
    if (!window)
        return;

    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd)
        return;

    const DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd,
                          DWMWA_WINDOW_CORNER_PREFERENCE,
                          &preference,
                          sizeof(preference));
}
#endif
#endif
}

int main(int argc, char* argv[])
{
    // Portable builds keep relative clips/log/config paths stable by
    // anchoring the process working directory to the executable folder.
    char exePath[MAX_PATH] = {};
    const DWORD exePathLength = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    if (exePathLength > 0 && exePathLength < MAX_PATH)
    {
        char* lastSlash = strrchr(exePath, '\\');
        if (!lastSlash)
            lastSlash = strrchr(exePath, '/');
        if (lastSlash)
        {
            *lastSlash = '\0';
            SetCurrentDirectoryA(exePath);
        }
    }

    // ── Init logger ──────────────────────────────────────────

    Logger::instance().setLogFile("ghostreplay.log");
    Logger::instance().setLevel(Logger::Debug);

    // ── Load config ──────────────────────────────────────────

    Config cfg;
    if (!cfg.load("ghostreplay.json"))
        cfg.load("ghostreplay.conf");   // legacy fallback
    cfg.parseArgs(argc, argv);

    if (cfg.log_file.empty())
        cfg.log_file = "ghostreplay.log";
    Logger::instance().setLogFile(cfg.log_file);
    Logger::instance().setLevel(loggerLevelFromConfig(cfg.log_level));

    if (cfg.output_dir.empty())
        cfg.output_dir = "clips";
    ensureOutputDirectory(cfg.output_dir);

    cfg.codec = lowerAscii(cfg.codec);
    if (cfg.codec != "h264")
    {
        LOG_WARNING("main: capture codec '" + cfg.codec + "' is not enabled in this release; using h264");
        cfg.codec = "h264";
    }

    // Refresh Windows startup registry key so JSON/manual changes are honored.
    Config::setStartupEnabled(cfg.launch_at_startup);

#ifdef HAS_QT6
    // ── Qt QML system-tray path ──────────────────────────

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickStyle::setStyle("Basic");
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication qapp(argc, argv);
    QApplication::setApplicationName("GhostReplay");
    QApplication::setOrganizationName("GhostReplay");
#ifdef GHOST_REPLAY_VERSION
    QApplication::setApplicationVersion(QStringLiteral(GHOST_REPLAY_VERSION));
#endif
    QApplication::setWindowIcon(QIcon(":/icons/ghostreplay.svg"));
    QApplication::setQuitOnLastWindowClosed(false);
    
    // ── Single Instance Check ────────────────────────────
    const QString serverName = QStringLiteral("GhostReplaySingleInstanceSocket");
    QLocalSocket socket;
    socket.connectToServer(serverName);
    std::cout << "[SINGLE_INSTANCE] Trying to connect to server: " << serverName.toStdString() << std::endl;
    if (socket.waitForConnected(500))
    {
        std::cout << "[SINGLE_INSTANCE] Connected! Exiting this instance." << std::endl;
        socket.write("show");
        socket.waitForBytesWritten(500);
        return 0; // Exit this instance
    }
    std::cout << "[SINGLE_INSTANCE] Connection failed, continuing initialization." << std::endl;

    // Host the server in this first instance
    QLocalServer::removeServer(serverName);
    QLocalServer* singleInstanceServer = new QLocalServer(&qapp);
    if (!singleInstanceServer->listen(serverName))
    {
        LOG_WARNING("main: Single instance socket server failed to listen: " + singleInstanceServer->errorString().toStdString());
    }

    // ── Create Backend before pipeline ───────────────────
    Backend* backend = new Backend(); // owned manually, deleted on exit
    ClipPreviewController* preview = new ClipPreviewController(); // owned manually, deleted on exit

    // ── Load QML FIRST so QML engine initializes before capture D3D11 device ──
    auto engine = std::make_unique<QQmlApplicationEngine>();
    QObject::connect(engine.get(), &QQmlApplicationEngine::warnings,
        [](const QList<QQmlError>& warnings)
        {
            for (const QQmlError& warning : warnings)
                std::cerr << "[QML] " << warning.toString().toStdString() << "\n";
        });
    WindowChromeController windowChrome;
    engine->rootContext()->setContextProperty("backend", backend);
    engine->rootContext()->setContextProperty("clipPreview", preview);
    engine->rootContext()->setContextProperty("windowChrome", &windowChrome);
    engine->addImageProvider("thumbnails", new ThumbnailProvider);

    engine->load(QUrl("qrc:/qml/MainWindow.qml"));
    if (engine->rootObjects().isEmpty())
    {
        std::cerr << "[FATAL] Failed to load QML UI.\n";
        delete backend;
        delete preview;
        return 1;
    }

    std::unique_ptr<ShellDpiEventFilter> shellDpiFilter;
    QWindow* mainWindow = qobject_cast<QWindow*>(engine->rootObjects().constFirst());
    if (mainWindow)
    {
        if (auto* quickWindow = qobject_cast<QQuickWindow*>(mainWindow))
            quickWindow->setColor(Qt::transparent);
#ifdef Q_OS_WIN
        applyWindowsRoundedCorners(mainWindow);
#endif
        fitWindowToAvailableScreen(mainWindow, qapp, true);
        wireWindowScreenClamping(mainWindow, qapp);
        shellDpiFilter = std::make_unique<ShellDpiEventFilter>(mainWindow, qapp);
        qapp.installNativeEventFilter(shellDpiFilter.get());
        windowChrome.setWindow(mainWindow);
        QTimer::singleShot(0, mainWindow, [mainWindow, &qapp, start_minimized = cfg.start_minimized]()
        {
            if (!start_minimized)
            {
                mainWindow->show();
                mainWindow->raise();
                mainWindow->requestActivate();
            }
            fitWindowToAvailableScreen(mainWindow, qapp, false);
        });

        if (singleInstanceServer)
        {
            QObject::connect(singleInstanceServer, &QLocalServer::newConnection, [singleInstanceServer, mainWindow]()
            {
                QLocalSocket* client = singleInstanceServer->nextPendingConnection();
                if (client)
                {
                    auto readAndProcess = [client, mainWindow]()
                    {
                        if (client->bytesAvailable() > 0)
                        {
                            QByteArray data = client->readAll();
                            if (data == "show")
                            {
                                if (mainWindow->windowStates() & Qt::WindowMinimized)
                                    mainWindow->showNormal();
                                else
                                    mainWindow->show();
                                mainWindow->raise();
                                mainWindow->requestActivate();
                            }
                            client->disconnectFromServer();
                            client->deleteLater();
                        }
                    };
                    QObject::connect(client, &QLocalSocket::readyRead, readAndProcess);
                    QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
                    readAndProcess();
                }
            });
        }
    }
    
    QQmlEngine::setObjectOwnership(backend, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(preview, QQmlEngine::CppOwnership);

    // ── THEN initialize capture pipeline ─────────────────
    GRApplication grapp(cfg);
    grapp.setMainWindow(mainWindow);
    grapp.setBackend(backend);
    if (!grapp.initialize())
    {
        std::cerr << "\n[FATAL] Failed to initialize capture pipeline.\n"
                  << "  Check ghostreplay.log for details.\n\n";
        delete backend;
        delete preview;
        return 1;
    }

    if (qEnvironmentVariableIsSet("GHOST_REPLAY_TRAY_RESTORE_SMOKE"))
    {
        QTimer::singleShot(500, [&grapp, &qapp]()
        {
            const bool ok = grapp.runTrayRestoreSmokeTest();
            std::cout << "[SMOKE] tray restore " << (ok ? "passed" : "failed") << "\n";
            qapp.exit(ok ? 0 : 2);
        });
    }

    int result = qapp.exec();
    if (shellDpiFilter)
        qapp.removeNativeEventFilter(shellDpiFilter.get());
    grapp.shutdown();
    engine.reset();
    delete backend;
    delete preview;
    return result;

#else
    // ── Console path ─────────────────────────────────────────

    // ── Init encoder config ──────────────────────────────────

    EncoderConfig enc_cfg;
    enc_cfg.qp_p          = cfg.cqp;
    enc_cfg.qp_i          = cfg.cqp;
    enc_cfg.gop_length    = cfg.keyframe_interval;
    enc_cfg.fps_num       = cfg.fps;
    enc_cfg.fps_den       = 1;
    enc_cfg.width         = cfg.capture_width;
    enc_cfg.height        = cfg.capture_height;
    enc_cfg.preset        = cfg.preset;
    enc_cfg.backend       = encoderConfigBackendFromString(cfg.encoder_backend);
    if (cfg.codec == "hevc") enc_cfg.codec = EncoderConfig::Codec::HEVC;

    // ── Init capture pipeline ────────────────────────────────

    CaptureManager capture_mgr;
    if (!capture_mgr.initialize(cfg.monitor_index, enc_cfg, cfg.capture_source))
    {
        LOG_ERROR("main: CaptureManager::initialize failed");
        std::cerr << "\n[FATAL] Failed to initialize capture pipeline.\n"
                  << "  Make sure you have:\n"
                  << "  - Windows 10 1903+\n"
                  << "  - A supported video encoder (NVENC, AMF, Quick Sync, or Media Foundation)\n"
                  << "  Check ghostreplay.log for details.\n\n";
        return 1;
    }

    // ── Init ring buffer ─────────────────────────────────────

    int64_t max_duration = static_cast<int64_t>(cfg.replay_duration_sec) * 1'000'000LL;
    RingBuffer ring_buffer(max_duration);

    // ── Init audio capture ───────────────────────────────────

    AudioCapture audio_cap;
    bool audio_active = false;
    if (cfg.audio.enabled)
    {
        if (audio_cap.initialize(cfg.audio))
        {
            audio_cap.start();
            audio_active = true;
            std::cout << "  [AUDIO]   WASAPI loopback active ("
                      << cfg.audio.sample_rate << "Hz, "
                      << cfg.audio.channels << "ch)\n";
        }
        else
        {
            LOG_WARNING("main: audio init failed — continuing without audio");
            std::cout << "  [AUDIO]   Failed to init audio — video only\n";
        }
    }

    // ── Start capture ────────────────────────────────────────

    capture_mgr.start(ring_buffer);

    std::cout << "\n  [RUNNING]  Recording to ring buffer...\n";
    std::cout << "  Press Alt+F10 to save the last " << cfg.replay_duration_sec
              << " seconds.\n";
    std::cout << "  Press Ctrl+C in this window to exit.\n\n";

    // ── Init hotkey ──────────────────────────────────────────

    HotkeyManager hotkey_mgr;

    hotkey_mgr.registerHotkey(cfg.hotkey.modifiers, cfg.hotkey.vk, [&]()
    {
        int64_t latest_pts = ring_buffer.latestPts();
        int64_t window_start = (latest_pts > max_duration)
            ? (latest_pts - max_duration) : 0;

        std::vector<RingBufferPacket> video_packets;
        const int64_t actual_start_us = ring_buffer.flushWindow(window_start, video_packets);

        if (video_packets.empty())
        {
            std::cout << "  [SKIP] Buffer is empty — nothing to save.\n";
            return;
        }

        std::vector<EncodedAudioPacket> audio_packets;
        if (audio_active)
            audio_cap.flushWindow(actual_start_us, audio_packets);

        for (auto& pkt : video_packets)
        {
            pkt.pts = pkt.pts >= actual_start_us ? pkt.pts - actual_start_us : 0;
            pkt.dts = pkt.dts >= actual_start_us ? pkt.dts - actual_start_us : 0;
        }

        if (!audio_packets.empty() && cfg.audio.sample_rate > 0)
        {
            const int64_t audio_start_samples = actual_start_us * cfg.audio.sample_rate / 1'000'000LL;
            for (auto& pkt : audio_packets)
                pkt.pts_samples = pkt.pts_samples >= audio_start_samples ? pkt.pts_samples - audio_start_samples : 0;
        }

        std::cout << "  [SAVING] v=" << video_packets.size()
                  << " a=" << audio_packets.size() << " ...\n";

        std::string output_path = ClipExporter::generateFilename(cfg.output_dir);
        auto seq_headers = capture_mgr.encoder()->sequenceHeaders();

        ClipExporter exporter;

        if (audio_active && !audio_packets.empty())
        {
            auto a_extra = audio_cap.codecConfig();
            if (!exporter.initialize(output_path, seq_headers,
                                      capture_mgr.width(), capture_mgr.height(),
                                      cfg.fps, 1,
                                      a_extra, cfg.audio.sample_rate,
                                      cfg.audio.channels))
            {
                std::cout << "  [FAIL] Exporter init failed.\n";
                return;
            }
        }
        else
        {
            if (!exporter.initialize(output_path, seq_headers,
                                      capture_mgr.width(), capture_mgr.height(),
                                      cfg.fps, 1))
            {
                std::cout << "  [FAIL] Exporter init failed.\n";
                return;
            }
        }

        if (!exporter.writeVideoPackets(video_packets))
        {
            std::cout << "  [FAIL] Video write failed.\n";
            return;
        }

        if (!audio_packets.empty() && !exporter.writeAudioPackets(audio_packets))
        {
            std::cout << "  [FAIL] Audio write failed.\n";
            return;
        }

        if (!exporter.finalize())
        {
            std::cout << "  [FAIL] Finalize failed.\n";
            return;
        }

        int64_t duration_ms = 0;
        if (!video_packets.empty())
            duration_ms = (video_packets.back().pts - video_packets.front().pts) / 1000;

        std::cout << "  [SAVED] " << output_path << "\n";
        std::cout << "          " << video_packets.size() << " frames, "
                  << duration_ms / 1000.0 << "s"
                  << (audio_active ? " + audio" : "") << "\n";
    });

    // ── Signal handlers ──────────────────────────────────────

    static std::atomic<bool> g_running{ true };

    signal(SIGINT, [](int) { g_running = false; });
    signal(SIGTERM, [](int) { g_running = false; });

    // ── Status reporting thread ──────────────────────────────

    std::thread status_thread([&]()
    {
        while (g_running)
        {
            std::this_thread::sleep_for(5s);
            std::cout << "\r  Buffer: " << ring_buffer.packetCount()
                      << " packets  |  ~"
                      << (ring_buffer.duration() / 1'000'000) << "s cached    \n";
        }
    });

    // ── Main loop: pump Windows messages ─────────────────────

    while (g_running)
    {
        if (!hotkey_mgr.pumpMessages())
            break;
        std::this_thread::sleep_for(10ms);
    }

    // ── Shutdown ─────────────────────────────────────────────

    std::cout << "\n  Shutting down...\n";

    if (audio_active)
        audio_cap.stop();

    capture_mgr.stop();

    if (status_thread.joinable())
        status_thread.join();

    std::cout << "  GhostReplay exited.\n\n";
    return 0;
#endif
}
