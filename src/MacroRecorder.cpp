#include "MacroRecorder.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

static MacroRecorder *g_recorder = nullptr;

static LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_recorder != nullptr) {
        const KBDLLHOOKSTRUCT *info = reinterpret_cast<const KBDLLHOOKSTRUCT *>(lParam);
        const bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        const bool injected = (info->flags & LLKHF_INJECTED) != 0;
        const bool extended = (info->flags & LLKHF_EXTENDED) != 0;
        const bool swallow = g_recorder->handleKeyboard(static_cast<int>(info->vkCode),
                                                        static_cast<int>(info->scanCode),
                                                        extended, keyUp, injected);
        if (swallow) {
            return 1;
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_recorder != nullptr) {
        const MSLLHOOKSTRUCT *info = reinterpret_cast<const MSLLHOOKSTRUCT *>(lParam);
        const bool injected = (info->flags & LLMHF_INJECTED) != 0;
        int wheelDelta = 0;
        if (wParam == WM_MOUSEWHEEL) {
            wheelDelta = static_cast<short>(HIWORD(info->mouseData));
        }
        g_recorder->handleMouse(static_cast<unsigned int>(wParam),
                                info->pt.x, info->pt.y, wheelDelta, injected);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

MacroRecorder::MacroRecorder(QObject *parent) : QObject(parent) {
}

MacroRecorder::~MacroRecorder() {
    removeHooks();
}

bool MacroRecorder::installHooks() {
    if (m_keyboardHook != nullptr || m_mouseHook != nullptr) {
        return true;
    }
    g_recorder = this;
    HINSTANCE module = GetModuleHandle(nullptr);
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, module, 0);
    m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, module, 0);
    if (m_keyboardHook == nullptr || m_mouseHook == nullptr) {
        removeHooks();
        return false;
    }
    return true;
}

void MacroRecorder::removeHooks() {
    if (m_keyboardHook != nullptr) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_keyboardHook));
        m_keyboardHook = nullptr;
    }
    if (m_mouseHook != nullptr) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_mouseHook));
        m_mouseHook = nullptr;
    }
    if (g_recorder == this) {
        g_recorder = nullptr;
    }
}

void MacroRecorder::startRecording() {
    m_events.clear();
    m_timer.restart();
    m_lastTimestamp = 0;
    m_recording = true;
}

void MacroRecorder::stopRecording() {
    m_recording = false;
}

void MacroRecorder::setHotkeys(int runVk, int stopVk, int recordVk) {
    m_runVk = runVk;
    m_stopVk = stopVk;
    m_recordVk = recordVk;
}

quint32 MacroRecorder::nextDelay() {
    const qint64 now = m_timer.elapsed();
    qint64 delay = now - m_lastTimestamp;
    if (delay < 0) {
        delay = 0;
    }
    m_lastTimestamp = now;
    return static_cast<quint32>(delay);
}

bool MacroRecorder::handleKeyboard(int vk, int scan, bool extended, bool keyUp, bool injected) {
    if (injected) {
        return false;
    }
    if (vk == m_runVk || vk == m_stopVk || vk == m_recordVk) {
        if (!keyUp) {
            if (vk == m_recordVk) {
                emit recordHotkeyPressed();
            } else if (vk == m_runVk) {
                emit runHotkeyPressed();
            } else if (vk == m_stopVk) {
                emit stopHotkeyPressed();
            }
        }
        return true;
    }
    if (!m_recording) {
        return false;
    }
    MacroEvent e;
    e.type = keyUp ? MacroEventType::KeyUp : MacroEventType::KeyDown;
    e.delayMs = nextDelay();
    e.vkCode = static_cast<quint32>(vk);
    e.scanCode = static_cast<quint32>(scan);
    e.extended = extended;
    m_events.append(e);
    emit eventRecorded(m_events.size());
    return false;
}

void MacroRecorder::handleMouse(unsigned int message, int x, int y, int wheelDelta, bool injected) {
    if (injected || !m_recording) {
        return;
    }
    MacroEvent e;
    e.delayMs = nextDelay();
    e.x = x;
    e.y = y;
    bool valid = true;
    switch (message) {
        case WM_MOUSEMOVE:
            e.type = MacroEventType::MouseMove;
            break;
        case WM_LBUTTONDOWN:
            e.type = MacroEventType::MouseDown;
            e.button = MouseButton::Left;
            break;
        case WM_LBUTTONUP:
            e.type = MacroEventType::MouseUp;
            e.button = MouseButton::Left;
            break;
        case WM_RBUTTONDOWN:
            e.type = MacroEventType::MouseDown;
            e.button = MouseButton::Right;
            break;
        case WM_RBUTTONUP:
            e.type = MacroEventType::MouseUp;
            e.button = MouseButton::Right;
            break;
        case WM_MBUTTONDOWN:
            e.type = MacroEventType::MouseDown;
            e.button = MouseButton::Middle;
            break;
        case WM_MBUTTONUP:
            e.type = MacroEventType::MouseUp;
            e.button = MouseButton::Middle;
            break;
        case WM_MOUSEWHEEL:
            e.type = MacroEventType::MouseWheel;
            e.wheelDelta = wheelDelta;
            break;
        default:
            valid = false;
            break;
    }
    if (!valid) {
        return;
    }
    m_events.append(e);
    emit eventRecorded(m_events.size());
}
