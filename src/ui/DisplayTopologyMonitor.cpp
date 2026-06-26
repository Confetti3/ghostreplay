#include "ui/DisplayTopologyMonitor.h"

#include <QGuiApplication>
#include <QScreen>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <vector>

#ifdef Q_OS_WIN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
#endif

namespace
{
QString compactDeviceName(QString value)
{
    if (value.startsWith(QStringLiteral("\\\\.\\")))
        value.remove(0, 4);
    return value.isEmpty() ? QStringLiteral("Display") : value;
}

QString displayLabel(const QString& deviceName, int index)
{
    const QString compact = compactDeviceName(deviceName);
    return compact == QStringLiteral("Display")
        ? QStringLiteral("Display %1").arg(index + 1)
        : compact;
}

double scaleFromDpi(int dpi)
{
    return std::max(0.5, static_cast<double>(dpi) / 96.0);
}

bool fuzzyScaleChanged(double a, double b)
{
    return std::abs(a - b) > 0.01;
}

QScreen* screenForDevice(const QString& deviceName, const QRect& bounds)
{
    QGuiApplication* app = qGuiApp;
    if (!app)
        return nullptr;

    const QString compact = compactDeviceName(deviceName);
    const auto screens = app->screens();
    for (QScreen* screen : screens)
    {
        if (!screen)
            continue;
        if (screen->name() == deviceName || screen->name() == compact)
            return screen;
    }

    for (QScreen* screen : screens)
    {
        if (screen && screen->geometry() == bounds)
            return screen;
    }

    return nullptr;
}

#ifdef Q_OS_WIN
struct MonitorRecord
{
    HMONITOR handle = nullptr;
    QString deviceName;
    QRect bounds;
    bool primary = false;
    int dpiX = 96;
    int dpiY = 96;
};

using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);

bool queryEffectiveDpi(HMONITOR monitor, int& dpiX, int& dpiY)
{
    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (!shcore)
        return false;

    auto* fn = reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor"));
    bool ok = false;
    if (fn)
    {
        UINT x = 96;
        UINT y = 96;
        if (SUCCEEDED(fn(monitor, 0, &x, &y)) && x > 0 && y > 0)
        {
            dpiX = static_cast<int>(x);
            dpiY = static_cast<int>(y);
            ok = true;
        }
    }

    FreeLibrary(shcore);
    return ok;
}

std::vector<MonitorRecord> enumerateMonitorRecords()
{
    std::vector<MonitorRecord> records;

    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
            auto* out = reinterpret_cast<std::vector<MonitorRecord>*>(data);

            MONITORINFOEXW info = {};
            info.cbSize = sizeof(info);
            if (!GetMonitorInfoW(monitor, &info))
                return TRUE;

            MonitorRecord record;
            record.handle = monitor;
            record.deviceName = QString::fromWCharArray(info.szDevice);
            record.bounds = QRect(info.rcMonitor.left,
                                  info.rcMonitor.top,
                                  info.rcMonitor.right - info.rcMonitor.left,
                                  info.rcMonitor.bottom - info.rcMonitor.top);
            record.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
            queryEffectiveDpi(monitor, record.dpiX, record.dpiY);

            if (QScreen* screen = screenForDevice(record.deviceName, record.bounds))
            {
                if (record.dpiX <= 0 || record.dpiX == 96)
                    record.dpiX = std::max(1, qRound(screen->logicalDotsPerInchX()));
                if (record.dpiY <= 0 || record.dpiY == 96)
                    record.dpiY = std::max(1, qRound(screen->logicalDotsPerInchY()));
            }

            out->push_back(record);
            return TRUE;
        }, reinterpret_cast<LPARAM>(&records));

    return records;
}
#endif
}

DisplayTopologyMonitor::DisplayTopologyMonitor(int monitorIndex, QObject* parent)
    : QObject(parent)
    , m_monitorIndex(std::max(0, monitorIndex))
{
    qRegisterMetaType<DisplaySnapshot>("DisplaySnapshot");
    qRegisterMetaType<DisplayChangeDecision>("DisplayChangeDecision");

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(800);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &DisplayTopologyMonitor::refreshNow);

    if (QGuiApplication* app = qGuiApp)
    {
        app->installNativeEventFilter(this);
        connect(app, &QGuiApplication::screenAdded,
                this, &DisplayTopologyMonitor::onScreenAdded);
        connect(app, &QGuiApplication::screenRemoved,
                this, &DisplayTopologyMonitor::scheduleRefresh);
        connectExistingScreens();
    }

    refreshNow();
}

DisplayTopologyMonitor::~DisplayTopologyMonitor()
{
    if (QGuiApplication* app = qGuiApp)
        app->removeNativeEventFilter(this);
}

void DisplayTopologyMonitor::setMonitorIndex(int monitorIndex)
{
    const int next = std::max(0, monitorIndex);
    if (m_monitorIndex == next)
        return;

    m_monitorIndex = next;
    scheduleRefresh();
}

int DisplayTopologyMonitor::monitorIndex() const
{
    return m_monitorIndex;
}

DisplaySnapshot DisplayTopologyMonitor::currentSnapshot() const
{
    return m_current;
}

DisplaySnapshot DisplayTopologyMonitor::previousSnapshot() const
{
    return m_previous;
}

DisplayChangeDecision DisplayTopologyMonitor::lastDecision(bool autoNativeCapture) const
{
    return compareSnapshots(m_previous, m_current, autoNativeCapture);
}

DisplaySnapshot DisplayTopologyMonitor::querySnapshot(int monitorIndex)
{
    DisplaySnapshot snapshot;
    snapshot.configuredMonitorIndex = std::max(0, monitorIndex);

#ifdef Q_OS_WIN
    const std::vector<MonitorRecord> records = enumerateMonitorRecords();
    if (records.empty())
        return snapshot;

    int selected = snapshot.configuredMonitorIndex;
    if (selected < 0 || selected >= static_cast<int>(records.size()))
    {
        snapshot.selectedMonitorMissing = true;
        selected = 0;
        for (int i = 0; i < static_cast<int>(records.size()); ++i)
        {
            if (records[static_cast<size_t>(i)].primary)
            {
                selected = i;
                break;
            }
        }
    }

    const MonitorRecord& record = records[static_cast<size_t>(selected)];
    snapshot.valid = true;
    snapshot.resolvedMonitorIndex = selected;
    snapshot.deviceName = record.deviceName;
    snapshot.label = displayLabel(record.deviceName, selected);
    snapshot.pixelBounds = record.bounds;
    snapshot.dpiX = record.dpiX > 0 ? record.dpiX : 96;
    snapshot.dpiY = record.dpiY > 0 ? record.dpiY : snapshot.dpiX;
    snapshot.scale = scaleFromDpi(snapshot.dpiX);
    return snapshot;
#else
    QGuiApplication* app = qGuiApp;
    if (!app || app->screens().isEmpty())
        return snapshot;

    const auto screens = app->screens();
    int selected = snapshot.configuredMonitorIndex;
    if (selected < 0 || selected >= screens.size())
    {
        snapshot.selectedMonitorMissing = true;
        selected = std::max(0, screens.indexOf(app->primaryScreen()));
    }

    QScreen* screen = screens.value(selected, app->primaryScreen());
    if (!screen)
        return snapshot;

    snapshot.valid = true;
    snapshot.resolvedMonitorIndex = selected;
    snapshot.deviceName = screen->name();
    snapshot.label = displayLabel(snapshot.deviceName, selected);
    snapshot.pixelBounds = screen->geometry();
    snapshot.dpiX = std::max(1, qRound(screen->logicalDotsPerInchX()));
    snapshot.dpiY = std::max(1, qRound(screen->logicalDotsPerInchY()));
    snapshot.scale = scaleFromDpi(snapshot.dpiX);
    return snapshot;
#endif
}

QVariantList DisplayTopologyMonitor::availableDisplays()
{
    QVariantList list;
#ifdef Q_OS_WIN
    const std::vector<MonitorRecord> records = enumerateMonitorRecords();
    for (int i = 0; i < static_cast<int>(records.size()); ++i)
    {
        const auto& record = records[static_cast<size_t>(i)];
        QVariantMap map;
        map["index"] = i;
        map["deviceName"] = record.deviceName;
        map["label"] = displayLabel(record.deviceName, i) + QStringLiteral(" (%1x%2)").arg(record.bounds.width()).arg(record.bounds.height());
        map["primary"] = record.primary;
        list.append(map);
    }
#else
    QGuiApplication* app = qGuiApp;
    if (app)
    {
        const auto screens = app->screens();
        for (int i = 0; i < screens.size(); ++i)
        {
            QScreen* screen = screens[i];
            if (!screen)
                continue;
            QVariantMap map;
            map["index"] = i;
            map["deviceName"] = screen->name();
            map["label"] = displayLabel(screen->name(), i) + QStringLiteral(" (%1x%2)").arg(screen->geometry().width()).arg(screen->geometry().height());
            map["primary"] = (screen == app->primaryScreen());
            list.append(map);
        }
    }
#endif
    return list;
}

DisplayChangeDecision DisplayTopologyMonitor::compareSnapshots(const DisplaySnapshot& previous,
                                                               const DisplaySnapshot& current,
                                                               bool autoNativeCapture)
{
    DisplayChangeDecision decision;
    decision.selectedMonitorMissing = current.selectedMonitorMissing;

    if (!previous.valid && !current.valid)
        return decision;

    if (!previous.valid || !current.valid)
    {
        decision.changed = true;
        decision.monitorChanged = true;
        decision.shouldRestartCapture = current.valid;
        decision.reason = QStringLiteral("display availability changed");
        return decision;
    }

    decision.monitorChanged =
        previous.resolvedMonitorIndex != current.resolvedMonitorIndex ||
        previous.deviceName != current.deviceName ||
        previous.selectedMonitorMissing != current.selectedMonitorMissing;
    decision.resolutionChanged = previous.pixelBounds.size() != current.pixelBounds.size();
    decision.dpiChanged =
        previous.dpiX != current.dpiX ||
        previous.dpiY != current.dpiY ||
        fuzzyScaleChanged(previous.scale, current.scale);

    decision.changed = decision.monitorChanged ||
        decision.resolutionChanged ||
        decision.dpiChanged;

    decision.shouldRestartCapture = decision.monitorChanged ||
        (autoNativeCapture && (decision.resolutionChanged || decision.dpiChanged));

    if (decision.monitorChanged)
        decision.reason = QStringLiteral("display changed");
    else if (decision.resolutionChanged)
        decision.reason = QStringLiteral("resolution changed");
    else if (decision.dpiChanged)
        decision.reason = QStringLiteral("DPI changed");

    return decision;
}

bool DisplayTopologyMonitor::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
    {
        const MSG* msg = static_cast<MSG*>(message);
        if (!msg)
            return false;

        switch (msg->message)
        {
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
        case WM_DEVICECHANGE:
        case WM_SETTINGCHANGE:
            scheduleRefresh();
            break;
        default:
            break;
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif

    return false;
}

void DisplayTopologyMonitor::scheduleRefresh()
{
    m_debounceTimer.start();
}

void DisplayTopologyMonitor::refreshNow()
{
    DisplaySnapshot next = querySnapshot(m_monitorIndex);
    next.generation = m_generation + 1;

    const DisplayChangeDecision decision = compareSnapshots(m_current, next, true);
    if (m_current.valid && !decision.changed)
        return;

    m_previous = m_current;
    m_current = next;
    m_generation = next.generation;
    emit topologySettled();
}

void DisplayTopologyMonitor::onScreenAdded(QScreen* screen)
{
    connectScreenSignals(screen);
    scheduleRefresh();
}

void DisplayTopologyMonitor::connectScreenSignals(QScreen* screen)
{
    if (!screen)
        return;

    connect(screen, &QScreen::geometryChanged,
            this, &DisplayTopologyMonitor::scheduleRefresh, Qt::UniqueConnection);
    connect(screen, &QScreen::availableGeometryChanged,
            this, &DisplayTopologyMonitor::scheduleRefresh, Qt::UniqueConnection);
    connect(screen, &QScreen::logicalDotsPerInchChanged,
            this, &DisplayTopologyMonitor::scheduleRefresh, Qt::UniqueConnection);
    connect(screen, &QScreen::physicalDotsPerInchChanged,
            this, &DisplayTopologyMonitor::scheduleRefresh, Qt::UniqueConnection);
}

void DisplayTopologyMonitor::connectExistingScreens()
{
    QGuiApplication* app = qGuiApp;
    if (!app)
        return;

    const auto screens = app->screens();
    for (QScreen* screen : screens)
        connectScreenSignals(screen);
}
