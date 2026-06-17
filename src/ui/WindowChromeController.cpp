#include "ui/WindowChromeController.h"

#include <QWindow>

WindowChromeController::WindowChromeController(QObject* parent)
    : QObject(parent)
{
}

void WindowChromeController::setWindow(QWindow* window)
{
    if (m_window == window)
        return;

    if (m_window)
        disconnect(m_window, nullptr, this, nullptr);

    m_window = window;

    if (m_window)
    {
        connect(m_window, &QWindow::windowStateChanged, this,
                [this](Qt::WindowState) { emitStateIfChanged(); });
        emitStateIfChanged();
    }
    else if (m_maximized)
    {
        m_maximized = false;
        emit maximizedChanged();
    }
}

bool WindowChromeController::maximized() const
{
    return m_maximized;
}

void WindowChromeController::minimize()
{
    if (m_window)
        m_window->showMinimized();
}

void WindowChromeController::maximizeRestore()
{
    if (!m_window)
        return;

    if ((m_window->windowStates() & Qt::WindowMaximized) || m_window->visibility() == QWindow::Maximized)
        m_window->showNormal();
    else
        m_window->showMaximized();

    emitStateIfChanged();
}

void WindowChromeController::close()
{
    if (m_window)
        m_window->close();
}

void WindowChromeController::startSystemMove()
{
    if (m_window)
        m_window->startSystemMove();
}

void WindowChromeController::startSystemResize(int edges)
{
    if (m_window)
        m_window->startSystemResize(static_cast<Qt::Edges>(edges));
}

void WindowChromeController::emitStateIfChanged()
{
    const bool next = m_window
        && (((m_window->windowStates() & Qt::WindowMaximized) != 0)
            || m_window->visibility() == QWindow::Maximized);
    if (m_maximized == next)
        return;

    m_maximized = next;
    emit maximizedChanged();
}
