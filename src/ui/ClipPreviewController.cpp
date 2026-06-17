#include "ui/ClipPreviewController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#  include <Windows.h>
#endif

namespace
{
QString formatSeconds(double seconds)
{
    return QString::number(qMax(0.0, seconds), 'f', 3);
}

void hideProcessWindow(QProcess& process)
{
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags &= ~CREATE_NEW_CONSOLE;
        args->flags |= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
    });
#else
    Q_UNUSED(process);
#endif
}
}

ClipPreviewController::ClipPreviewController(QObject* parent)
    : QObject(parent)
{
    const QString cache_root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString base_dir = cache_root.isEmpty()
        ? QDir::temp().filePath("GhostReplay")
        : QDir(cache_root).filePath("preview");
    QDir().mkpath(base_dir);
    m_frame_dir = base_dir;
    m_frame_path = QDir(base_dir).filePath("clip-preview.jpg");
}

ClipPreviewController::~ClipPreviewController()
{
    clear();
}

QString ClipPreviewController::frameUrl() const { return m_frame_url; }
bool ClipPreviewController::hasFrame() const { return m_has_frame; }
bool ClipPreviewController::busy() const { return m_busy; }
QString ClipPreviewController::error() const { return m_error; }
double ClipPreviewController::currentTimeSec() const { return m_current_time_sec; }

void ClipPreviewController::loadClip(const QString& path)
{
    if (m_clip_path == path)
        return;

    clear();
    m_clip_path = path;
    if (!m_clip_path.isEmpty())
        requestFrame(0.0);
}

void ClipPreviewController::requestFrame(double seconds)
{
    if (m_clip_path.isEmpty())
    {
        setError("No clip selected.");
        return;
    }

    const double requested = qMax(0.0, seconds);
    if (m_busy)
    {
        m_pending_time_sec = requested;
        return;
    }

    startFrameRequest(requested);
}

void ClipPreviewController::clear()
{
    m_pending_time_sec = -1.0;
    if (m_process)
    {
        disconnect(m_process, nullptr, this, nullptr);
        if (m_process->state() != QProcess::NotRunning)
            m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_stderr.clear();
    m_clip_path.clear();
    m_frame_url.clear();
    m_has_frame = false;
    m_current_time_sec = 0.0;
    setBusy(false);
    setError(QString());
    emit frameChanged();
}

QString ClipPreviewController::locateFfmpegExecutable() const
{
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QString local = QDir(app_dir).filePath("ffmpeg.exe");
    if (QFileInfo::exists(local))
        return local;

    const QString in_path = QStandardPaths::findExecutable("ffmpeg");
    if (!in_path.isEmpty())
        return in_path;

    const QString env_root = qEnvironmentVariable("FFMPEG_DIR");
    if (!env_root.isEmpty())
    {
        const QString env_path = QDir(env_root).filePath("bin/ffmpeg.exe");
        if (QFileInfo::exists(env_path))
            return env_path;
    }

    return QString();
}

void ClipPreviewController::startFrameRequest(double seconds)
{
    const QString ffmpeg = locateFfmpegExecutable();
    if (ffmpeg.isEmpty())
    {
        setError("FFmpeg executable was not found next to the app or in PATH.");
        return;
    }

    if (!QFileInfo::exists(m_clip_path))
    {
        setError("Selected clip no longer exists.");
        return;
    }

    m_stderr.clear();
    setError(QString());
    setBusy(true);

    m_current_time_sec = seconds;
    QFile::remove(m_frame_path);
    ++m_generation;
    m_frame_path = QDir(m_frame_dir).filePath(QString("clip-preview-%1.jpg").arg(m_generation));

    m_process = new QProcess(this);
    hideProcessWindow(*m_process);
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        if (m_process)
            m_stderr += m_process->readAllStandardError();
    });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &ClipPreviewController::onProcessFinished);

    QStringList args{
        "-y",
        "-v", "error",
        "-ss", formatSeconds(seconds),
        "-i", m_clip_path,
        "-frames:v", "1",
        "-vf", "scale='min(1280,iw)':-2",
        "-q:v", "2",
        m_frame_path
    };
    m_process->start(ffmpeg, args);
}

void ClipPreviewController::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* finished_process = m_process;
    m_process = nullptr;
    if (finished_process)
        finished_process->deleteLater();

    setBusy(false);

    const bool ok = exitStatus == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(m_frame_path);
    if (ok)
    {
        m_frame_url = QUrl::fromLocalFile(m_frame_path).toString();
        m_has_frame = true;
        emit frameChanged();
    }
    else
    {
        const QString detail = QString::fromUtf8(m_stderr).trimmed();
        setError(detail.isEmpty() ? "Preview unavailable for this clip." : detail);
    }

    if (m_pending_time_sec >= 0.0)
    {
        const double next = m_pending_time_sec;
        m_pending_time_sec = -1.0;
        startFrameRequest(next);
    }
}

void ClipPreviewController::setBusy(bool value)
{
    if (m_busy == value)
        return;
    m_busy = value;
    emit busyChanged();
}

void ClipPreviewController::setError(const QString& value)
{
    if (m_error == value)
        return;
    m_error = value;
    emit errorChanged();
}
