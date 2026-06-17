#include "ui/MpvPreviewController.h"
#include "util/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QPoint>
#include <QPointF>
#include <QQuickItem>
#include <QStringList>
#include <QWindow>
#include <algorithm>
#include <cstdint>
#include <string>

#ifdef Q_OS_WIN
#  include <Windows.h>
#endif

namespace
{
constexpr int MPV_FORMAT_STRING = 1;
constexpr int MPV_FORMAT_FLAG = 3;
constexpr int MPV_FORMAT_DOUBLE = 4;

using mpv_handle = void;
using mpv_create_fn = mpv_handle* (*)();
using mpv_initialize_fn = int (*)(mpv_handle*);
using mpv_terminate_destroy_fn = void (*)(mpv_handle*);
using mpv_command_fn = int (*)(mpv_handle*, const char**);
using mpv_set_option_string_fn = int (*)(mpv_handle*, const char*, const char*);
using mpv_set_property_fn = int (*)(mpv_handle*, const char*, int, void*);
using mpv_get_property_fn = int (*)(mpv_handle*, const char*, int, void*);

struct MpvApi
{
    mpv_create_fn create = nullptr;
    mpv_initialize_fn initialize = nullptr;
    mpv_terminate_destroy_fn terminate_destroy = nullptr;
    mpv_command_fn command = nullptr;
    mpv_set_option_string_fn set_option_string = nullptr;
    mpv_set_property_fn set_property = nullptr;
    mpv_get_property_fn get_property = nullptr;
};

MpvApi g_mpv;

#ifdef Q_OS_WIN
const wchar_t* kMpvWindowClass = L"GhostReplayMpvHost";

LRESULT CALLBACK mpvHostWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_ERASEBKGND)
    {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void registerMpvHostClass()
{
    static bool registered = false;
    if (registered)
        return;

    WNDCLASSW wc{};
    wc.lpfnWndProc = mpvHostWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kMpvWindowClass;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassW(&wc);
    registered = true;
}
#endif
}

MpvPreviewController::MpvPreviewController(QObject* parent)
    : QObject(parent)
{
    m_available = ensureMpvLoaded();
    connect(&m_poll_timer, &QTimer::timeout, this, &MpvPreviewController::pollState);
    m_poll_timer.setInterval(100);
    emit availabilityChanged();
}

MpvPreviewController::~MpvPreviewController()
{
    destroyPlayer();
    destroyVideoWindow();
#ifdef Q_OS_WIN
    if (m_library)
        FreeLibrary(static_cast<HMODULE>(m_library));
#endif
}

bool MpvPreviewController::ensureMpvLoaded()
{
    if (m_library)
        return true;

#ifdef Q_OS_WIN
    const QStringList names{ "mpv-2.dll", "mpv.dll", "libmpv-2.dll" };
    const QString app_dir = QCoreApplication::applicationDirPath();
    QStringList candidates;
    for (const QString& name : names)
        candidates.append(QDir(app_dir).filePath(name));
    candidates.append(names);

    HMODULE lib = nullptr;
    DWORD last_error = ERROR_SUCCESS;
    QString attempted;
    for (const QString& candidate : candidates)
    {
        attempted += candidate + "; ";
        const std::wstring wide = QDir::toNativeSeparators(candidate).toStdWString();
        lib = LoadLibraryExW(wide.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (lib)
        {
            LOG_INFO("MpvPreviewController: loaded " + candidate.toStdString());
            break;
        }
        last_error = GetLastError();
    }
    if (!lib)
    {
        LOG_WARNING("MpvPreviewController: libmpv load failed, error=" + std::to_string(last_error) +
            ", tried: " + attempted.toStdString());
        setError("Preview unavailable: mpv-2.dll was not found next to Ghost Replay.");
        return false;
    }

    m_library = lib;
    g_mpv.create = reinterpret_cast<mpv_create_fn>(GetProcAddress(lib, "mpv_create"));
    g_mpv.initialize = reinterpret_cast<mpv_initialize_fn>(GetProcAddress(lib, "mpv_initialize"));
    g_mpv.terminate_destroy = reinterpret_cast<mpv_terminate_destroy_fn>(GetProcAddress(lib, "mpv_terminate_destroy"));
    g_mpv.command = reinterpret_cast<mpv_command_fn>(GetProcAddress(lib, "mpv_command"));
    g_mpv.set_option_string = reinterpret_cast<mpv_set_option_string_fn>(GetProcAddress(lib, "mpv_set_option_string"));
    g_mpv.set_property = reinterpret_cast<mpv_set_property_fn>(GetProcAddress(lib, "mpv_set_property"));
    g_mpv.get_property = reinterpret_cast<mpv_get_property_fn>(GetProcAddress(lib, "mpv_get_property"));

    const bool ok = g_mpv.create && g_mpv.initialize && g_mpv.terminate_destroy &&
        g_mpv.command && g_mpv.set_option_string && g_mpv.set_property && g_mpv.get_property;
    if (!ok)
    {
        LOG_WARNING("MpvPreviewController: loaded libmpv is missing required API symbols");
        setError("The loaded libmpv DLL is missing required API symbols.");
        return false;
    }
    return true;
#else
    setError("libmpv preview is currently implemented for Windows builds only.");
    return false;
#endif
}

bool MpvPreviewController::ensureWindow(QObject* windowObject)
{
    QWindow* window = qobject_cast<QWindow*>(windowObject);
    if (!window)
    {
        setError("Unable to attach mpv preview to the application window.");
        return false;
    }

    m_window = window;
#ifdef Q_OS_WIN
    if (m_hwnd)
        return true;

    registerMpvHostClass();
    HWND parent = reinterpret_cast<HWND>(window->winId());
    HWND hwnd = CreateWindowExW(
        0,
        kMpvWindowClass,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        m_x,
        m_y,
        std::max(1, m_width),
        std::max(1, m_height),
        parent,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (!hwnd)
    {
        setError("Unable to create the native libmpv preview window.");
        return false;
    }

    m_hwnd = hwnd;
    return true;
#else
    return false;
#endif
}

void MpvPreviewController::attach(QObject* windowObject, QObject* itemObject, int width, int height)
{
    QWindow* window = qobject_cast<QWindow*>(windowObject);
    QQuickItem* item = qobject_cast<QQuickItem*>(itemObject);
    if (!window || !item)
    {
        setError("Unable to attach mpv preview to the preview surface.");
        return;
    }

    const QPointF global_pos = item->mapToGlobal(QPointF(0, 0));
    const QPoint client_pos = window->mapFromGlobal(QPoint(
        qRound(global_pos.x()),
        qRound(global_pos.y())));

    m_x = client_pos.x();
    m_y = client_pos.y();
    m_width = width;
    m_height = height;
    if (!m_available)
    {
        destroyVideoWindow();
        return;
    }
    ensureWindow(windowObject);
    setVideoGeometry(m_x, m_y, width, height);
}

void MpvPreviewController::setVideoGeometry(int x, int y, int width, int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
#ifdef Q_OS_WIN
    if (m_hwnd)
    {
        SetWindowPos(static_cast<HWND>(m_hwnd), nullptr, m_x, m_y,
            std::max(1, m_width), std::max(1, m_height),
            SWP_NOZORDER | SWP_SHOWWINDOW);
    }
#endif
}

bool MpvPreviewController::ensurePlayer()
{
    if (m_mpv)
        return true;

    if (!ensureMpvLoaded())
        return false;
    if (!m_hwnd)
    {
        setError("Preview surface is not ready yet.");
        return false;
    }

    m_mpv = g_mpv.create();
    if (!m_mpv)
    {
        setError("Unable to create libmpv player.");
        return false;
    }

    g_mpv.set_option_string(m_mpv, "terminal", "no");
    g_mpv.set_option_string(m_mpv, "input-default-bindings", "no");
    g_mpv.set_option_string(m_mpv, "input-vo-keyboard", "no");
    g_mpv.set_option_string(m_mpv, "osc", "no");
#ifdef Q_OS_WIN
    const std::string wid = std::to_string(reinterpret_cast<std::uintptr_t>(m_hwnd));
    g_mpv.set_option_string(m_mpv, "wid", wid.c_str());
#endif

    if (g_mpv.initialize(m_mpv) < 0)
    {
        destroyPlayer();
        setError("Unable to initialize libmpv.");
        return false;
    }

    setMpvPropertyDouble("volume", m_volume * 100.0);
    m_poll_timer.start();
    return true;
}

void MpvPreviewController::destroyPlayer()
{
    m_poll_timer.stop();
    if (m_mpv)
    {
        g_mpv.terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
    setLoaded(false);
    setPlaying(false);
    setPosition(0.0);
    setDuration(0.0);
}

void MpvPreviewController::destroyVideoWindow()
{
#ifdef Q_OS_WIN
    if (m_hwnd)
    {
        DestroyWindow(static_cast<HWND>(m_hwnd));
        m_hwnd = nullptr;
    }
#endif
}

void MpvPreviewController::loadClip(const QString& path)
{
    if (!QFileInfo::exists(path))
    {
        clear();
        setError("Selected clip no longer exists.");
        return;
    }

    if (!ensurePlayer())
        return;

    const QByteArray encoded = QFileInfo(path).absoluteFilePath().toUtf8();
    const char* args[] = { "loadfile", encoded.constData(), "replace", nullptr };
    if (!command(args))
    {
        setError("libmpv could not load this clip.");
        return;
    }

    setError(QString());
    setLoaded(true);
    setPlaying(false);
    setPosition(0.0);
}

void MpvPreviewController::clear()
{
    if (m_mpv)
    {
        const char* args[] = { "stop", nullptr };
        command(args);
    }
    setError(QString());
    setLoaded(false);
    setPlaying(false);
    setPosition(0.0);
    setDuration(0.0);
}

void MpvPreviewController::play()
{
    if (m_mpv && setMpvPropertyFlag("pause", false))
        setPlaying(true);
}

void MpvPreviewController::pause()
{
    if (m_mpv && setMpvPropertyFlag("pause", true))
        setPlaying(false);
}

void MpvPreviewController::seek(double seconds)
{
    if (!m_mpv)
        return;
    const QByteArray value = QByteArray::number(std::max(0.0, seconds), 'f', 3);
    const char* args[] = { "seek", value.constData(), "absolute", "exact", nullptr };
    command(args);
    setPosition(std::max(0.0, seconds));
}

void MpvPreviewController::setVolume(double value)
{
    const double clamped = std::clamp(value, 0.0, 1.0);
    if (qFuzzyCompare(m_volume + 1.0, clamped + 1.0))
        return;
    m_volume = clamped;
    if (m_mpv)
        setMpvPropertyDouble("volume", m_volume * 100.0);
    emit volumeChanged();
}

void MpvPreviewController::pollState()
{
    if (!m_mpv)
        return;

    double position = 0.0;
    if (getMpvPropertyDouble("time-pos", position))
        setPosition(position);

    double duration = 0.0;
    if (getMpvPropertyDouble("duration", duration))
        setDuration(duration);

    bool paused = true;
    if (getMpvPropertyFlag("pause", paused))
        setPlaying(!paused);
}

void MpvPreviewController::setError(const QString& error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void MpvPreviewController::setLoaded(bool loaded)
{
    if (m_loaded == loaded)
        return;
    m_loaded = loaded;
    emit loadedChanged();
}

void MpvPreviewController::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged();
}

void MpvPreviewController::setPosition(double position)
{
    if (qAbs(m_position_sec - position) < 0.05)
        return;
    m_position_sec = position;
    emit positionChanged();
}

void MpvPreviewController::setDuration(double duration)
{
    if (qAbs(m_duration_sec - duration) < 0.05)
        return;
    m_duration_sec = duration;
    emit durationChanged();
}

bool MpvPreviewController::command(const char* const* args)
{
    return m_mpv && g_mpv.command(m_mpv, const_cast<const char**>(args)) >= 0;
}

bool MpvPreviewController::setMpvPropertyFlag(const char* name, bool value)
{
    int flag = value ? 1 : 0;
    return m_mpv && g_mpv.set_property(m_mpv, name, MPV_FORMAT_FLAG, &flag) >= 0;
}

bool MpvPreviewController::setMpvPropertyDouble(const char* name, double value)
{
    return m_mpv && g_mpv.set_property(m_mpv, name, MPV_FORMAT_DOUBLE, &value) >= 0;
}

bool MpvPreviewController::getMpvPropertyDouble(const char* name, double& value) const
{
    return m_mpv && g_mpv.get_property(m_mpv, name, MPV_FORMAT_DOUBLE, &value) >= 0;
}

bool MpvPreviewController::getMpvPropertyFlag(const char* name, bool& value) const
{
    int flag = 0;
    const bool ok = m_mpv && g_mpv.get_property(m_mpv, name, MPV_FORMAT_FLAG, &flag) >= 0;
    if (ok)
        value = flag != 0;
    return ok;
}
