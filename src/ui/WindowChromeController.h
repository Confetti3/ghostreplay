#pragma once

#include <QObject>
#include <Qt>

class QWindow;
class QEvent;

class WindowChromeController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool maximized READ maximized NOTIFY maximizedChanged)
    Q_PROPERTY(qreal mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(qreal mouseY READ mouseY NOTIFY mouseYChanged)
    Q_PROPERTY(bool mouseActive READ mouseActive NOTIFY mouseActiveChanged)

public:
    explicit WindowChromeController(QObject* parent = nullptr);

    void setWindow(QWindow* window);

    bool maximized() const;
    qreal mouseX() const { return m_mouseX; }
    qreal mouseY() const { return m_mouseY; }
    bool mouseActive() const { return m_mouseActive; }

    Q_INVOKABLE void minimize();
    Q_INVOKABLE void maximizeRestore();
    Q_INVOKABLE void close();
    Q_INVOKABLE void startSystemMove();
    Q_INVOKABLE void startSystemResize(int edges);

signals:
    void maximizedChanged();
    void mouseXChanged();
    void mouseYChanged();
    void mouseActiveChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void emitStateIfChanged();

    QWindow* m_window = nullptr;
    bool m_maximized = false;
    qreal m_mouseX = 0.0;
    qreal m_mouseY = 0.0;
    bool m_mouseActive = false;
};
