#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>

class QWindow;

class MpvPreviewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(double positionSec READ positionSec NOTIFY positionChanged)
    Q_PROPERTY(double durationSec READ durationSec NOTIFY durationChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit MpvPreviewController(QObject* parent = nullptr);
    ~MpvPreviewController() override;

    bool available() const { return m_available; }
    bool loaded() const { return m_loaded; }
    bool playing() const { return m_playing; }
    double positionSec() const { return m_position_sec; }
    double durationSec() const { return m_duration_sec; }
    double volume() const { return m_volume; }
    QString error() const { return m_error; }

    Q_INVOKABLE void attach(QObject* windowObject, QObject* itemObject, int width, int height);
    Q_INVOKABLE void setVideoGeometry(int x, int y, int width, int height);
    Q_INVOKABLE void loadClip(const QString& path);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void seek(double seconds);
    Q_INVOKABLE void setVolume(double value);

signals:
    void availabilityChanged();
    void loadedChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void errorChanged();

private slots:
    void pollState();

private:
    bool ensureMpvLoaded();
    bool ensureWindow(QObject* windowObject);
    bool ensurePlayer();
    void destroyPlayer();
    void destroyVideoWindow();
    void setError(const QString& error);
    void setLoaded(bool loaded);
    void setPlaying(bool playing);
    void setPosition(double position);
    void setDuration(double duration);
    bool command(const char* const* args);
    bool setMpvPropertyFlag(const char* name, bool value);
    bool setMpvPropertyDouble(const char* name, double value);
    bool getMpvPropertyDouble(const char* name, double& value) const;
    bool getMpvPropertyFlag(const char* name, bool& value) const;

    QPointer<QWindow> m_window;
    QTimer m_poll_timer;
    bool m_available = false;
    bool m_loaded = false;
    bool m_playing = false;
    double m_position_sec = 0.0;
    double m_duration_sec = 0.0;
    double m_volume = 1.0;
    QString m_error;

    void* m_library = nullptr;
    void* m_mpv = nullptr;
    void* m_hwnd = nullptr;
    int m_x = 0;
    int m_y = 0;
    int m_width = 0;
    int m_height = 0;
};
