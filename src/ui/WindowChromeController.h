#pragma once

#include <QObject>
#include <Qt>

class QWindow;

class WindowChromeController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool maximized READ maximized NOTIFY maximizedChanged)

public:
    explicit WindowChromeController(QObject* parent = nullptr);

    void setWindow(QWindow* window);

    bool maximized() const;

    Q_INVOKABLE void minimize();
    Q_INVOKABLE void maximizeRestore();
    Q_INVOKABLE void close();
    Q_INVOKABLE void startSystemMove();
    Q_INVOKABLE void startSystemResize(int edges);

signals:
    void maximizedChanged();

private:
    void emitStateIfChanged();

    QWindow* m_window = nullptr;
    bool m_maximized = false;
};
