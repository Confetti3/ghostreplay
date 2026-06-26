#pragma once

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QObject>
#include <QRect>
#include <QString>
#include <QTimer>

class QScreen;

struct DisplaySnapshot
{
    bool valid = false;
    bool selectedMonitorMissing = false;
    int configuredMonitorIndex = 0;
    int resolvedMonitorIndex = 0;
    QString deviceName;
    QString label;
    QRect pixelBounds;
    int dpiX = 96;
    int dpiY = 96;
    double scale = 1.0;
    quint64 generation = 0;
};

struct DisplayChangeDecision
{
    bool changed = false;
    bool monitorChanged = false;
    bool resolutionChanged = false;
    bool dpiChanged = false;
    bool selectedMonitorMissing = false;
    bool shouldRestartCapture = false;
    QString reason;
};

class DisplayTopologyMonitor : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit DisplayTopologyMonitor(int monitorIndex, QObject* parent = nullptr);
    ~DisplayTopologyMonitor() override;

    void setMonitorIndex(int monitorIndex);
    int monitorIndex() const;

    DisplaySnapshot currentSnapshot() const;
    DisplaySnapshot previousSnapshot() const;
    DisplayChangeDecision lastDecision(bool autoNativeCapture) const;

    static DisplaySnapshot querySnapshot(int monitorIndex);
    static DisplayChangeDecision compareSnapshots(const DisplaySnapshot& previous,
                                                   const DisplaySnapshot& current,
                                                   bool autoNativeCapture);
    static QVariantList availableDisplays();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void topologySettled();

private slots:
    void scheduleRefresh();
    void refreshNow();
    void onScreenAdded(QScreen* screen);

private:
    void connectScreenSignals(QScreen* screen);
    void connectExistingScreens();

    int m_monitorIndex = 0;
    quint64 m_generation = 0;
    DisplaySnapshot m_previous;
    DisplaySnapshot m_current;
    QTimer m_debounceTimer;
};

Q_DECLARE_METATYPE(DisplaySnapshot)
Q_DECLARE_METATYPE(DisplayChangeDecision)
