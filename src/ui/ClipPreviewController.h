#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class ClipPreviewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString frameUrl READ frameUrl NOTIFY frameChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY frameChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(double currentTimeSec READ currentTimeSec NOTIFY frameChanged)

public:
    explicit ClipPreviewController(QObject* parent = nullptr);
    ~ClipPreviewController() override;

    QString frameUrl() const;
    bool hasFrame() const;
    bool busy() const;
    QString error() const;
    double currentTimeSec() const;

    Q_INVOKABLE void loadClip(const QString& path);
    Q_INVOKABLE void requestFrame(double seconds);
    Q_INVOKABLE void clear();

signals:
    void frameChanged();
    void busyChanged();
    void errorChanged();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString locateFfmpegExecutable() const;
    void startFrameRequest(double seconds);
    void setBusy(bool value);
    void setError(const QString& value);

    QString m_clip_path;
    QString m_frame_dir;
    QString m_frame_path;
    QString m_frame_url;
    QString m_error;
    QProcess* m_process = nullptr;
    QByteArray m_stderr;
    double m_current_time_sec = 0.0;
    double m_pending_time_sec = -1.0;
    int m_generation = 0;
    bool m_busy = false;
    bool m_has_frame = false;
};
