#include "hotkey/HotkeyManager.h"
#include "util/Logger.h"

HotkeyManager* HotkeyManager::s_instance = nullptr;

HotkeyManager::HotkeyManager()
{
    s_instance = this;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = windowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"GhostReplayHotkeyWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        0, L"GhostReplayHotkeyWindow", L"GhostReplay",
        WS_OVERLAPPED, 0, 0, 0, 0,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!m_hwnd)
    {
        LOG_ERROR("HotkeyManager: failed to create message window");
    }
}

HotkeyManager::~HotkeyManager()
{
    stop();
    if (m_hwnd)
    {
        if (m_registered)
        {
            UnregisterHotKey(m_hwnd, m_hotkey_id);
        }
        DestroyWindow(m_hwnd);
    }
    s_instance = nullptr;
}

bool HotkeyManager::registerHotkey(UINT modifiers, UINT vk, HotkeyCallback callback)
{
    if (!m_hwnd) return false;

    m_callback = std::move(callback);

    BOOL ok = RegisterHotKey(m_hwnd, m_hotkey_id, modifiers, vk);
    if (!ok)
    {
        DWORD err = GetLastError();
        LOG_ERROR("HotkeyManager: RegisterHotKey failed, error=" + std::to_string(err));
        return false;
    }

    m_registered = true;
    LOG_INFO("HotkeyManager: hotkey registered (mod=" + std::to_string(modifiers) +
             " vk=" + std::to_string(vk) + ")");
    return true;
}

bool HotkeyManager::pumpMessages()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_running = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return m_running;
}

void HotkeyManager::runMessageLoop()
{
    LOG_INFO("HotkeyManager: entering message loop");
    m_running = true;

    MSG msg;
    while (m_running && GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    LOG_INFO("HotkeyManager: message loop exited");
}

void HotkeyManager::stop()
{
    m_running = false;
    if (m_hwnd)
        PostMessageW(m_hwnd, WM_NULL, 0, 0);
}

LRESULT CALLBACK HotkeyManager::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_HOTKEY && s_instance)
    {
        LOG_INFO("HotkeyManager: hotkey pressed");
        if (s_instance->m_callback)
        {
            s_instance->m_callback();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
