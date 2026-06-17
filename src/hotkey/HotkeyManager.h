#pragma once

#include <Windows.h>
#include <functional>
#include <atomic>

// Registers a global hotkey and runs a Windows message pump.
class HotkeyManager
{
public:
    using HotkeyCallback = std::function<void()>;

    HotkeyManager();
    ~HotkeyManager();

    // Register a hotkey combination.
    // modifiers: MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN (from Windows.h)
    // vk: virtual key code (e.g., VK_F10)
    // Returns false if registration fails (another app holds the key).
    bool registerHotkey(UINT modifiers, UINT vk, HotkeyCallback callback);

    // Process pending Windows messages (non-blocking).
    // Returns false if WM_QUIT was received.
    bool pumpMessages();

    // Run a blocking message loop until stop() is called.
    void runMessageLoop();

    // Signal the message loop to exit.
    void stop();

    // Check if the hotkey is currently registered.
    bool isRegistered() const { return m_registered.load(); }

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    UINT m_hotkey_id = 1;
    HotkeyCallback m_callback;
    std::atomic<bool> m_registered{ false };
    std::atomic<bool> m_running{ true };

    static HotkeyManager* s_instance; // For window proc dispatch
};
