#include "MacroPlayer.h"
#include <QRandomGenerator>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

MacroPlayer::MacroPlayer(QObject *parent) : QThread(parent) {
}

MacroPlayer::~MacroPlayer() {
    requestStop();
    wait();
}

void MacroPlayer::configure(const QVector<MacroEvent> &events, int repeatCount, bool infinite, bool instant) {
    m_events = events;
    m_repeatCount = repeatCount < 1 ? 1 : repeatCount;
    m_infinite = infinite;
    m_instant = instant;
}

void MacroPlayer::requestStop() {
    m_stop.store(true);
}

bool MacroPlayer::interruptibleSleep(quint32 ms) {
    quint32 remaining = ms;
    while (remaining > 0) {
        if (m_stop.load()) {
            return false;
        }
        const quint32 chunk = remaining > 20 ? 20 : remaining;
        QThread::msleep(chunk);
        remaining -= chunk;
    }
    return !m_stop.load();
}

void MacroPlayer::run() {
    m_stop.store(false);
    emit playbackStarted();

    const int total = m_infinite ? 0 : m_repeatCount;
    int iteration = 0;
    while (!m_stop.load()) {
        if (!m_infinite && iteration >= m_repeatCount) {
            break;
        }
        ++iteration;
        emit iterationChanged(iteration, total);
        for (const MacroEvent &e : m_events) {
            if (m_stop.load()) {
                break;
            }
            if (!m_instant && !interruptibleSleep(e.delayMs)) {
                break;
            }
            if (m_stop.load()) break;
            playEvent(e);
            if (e.randomDelayMaxMs > 0) {
                const quint32 extra = QRandomGenerator::global()->bounded(e.randomDelayMaxMs + 1);
                if (!interruptibleSleep(extra)) {
                    break;
                }
            }
        }
    }

    emit playbackFinished();
}

void MacroPlayer::playEvent(const MacroEvent &e) {
    INPUT input;
    ZeroMemory(&input, sizeof(input));

    switch (e.type) {
        case MacroEventType::MouseMove:
        case MacroEventType::MouseDown:
        case MacroEventType::MouseUp:
        case MacroEventType::MouseWheel: {
            input.type = INPUT_MOUSE;

            int vsX = GetSystemMetrics(SM_XVIRTUALSCREEN);
            int vsY = GetSystemMetrics(SM_YVIRTUALSCREEN);
            int vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            int vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            if (vsW < 2) {
                vsW = 2;
            }
            if (vsH < 2) {
                vsH = 2;
            }

            const double nx = static_cast<double>(e.x - vsX) * 65535.0 / static_cast<double>(vsW - 1);
            const double ny = static_cast<double>(e.y - vsY) * 65535.0 / static_cast<double>(vsH - 1);
            input.mi.dx = static_cast<LONG>(nx);
            input.mi.dy = static_cast<LONG>(ny);
            input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;

            if (e.type == MacroEventType::MouseDown) {
                if (e.button == MouseButton::Left) {
                    input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                } else if (e.button == MouseButton::Right) {
                    input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                } else if (e.button == MouseButton::Middle) {
                    input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                }
            } else if (e.type == MacroEventType::MouseUp) {
                if (e.button == MouseButton::Left) {
                    input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                } else if (e.button == MouseButton::Right) {
                    input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                } else if (e.button == MouseButton::Middle) {
                    input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                }
            } else if (e.type == MacroEventType::MouseWheel) {
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = static_cast<DWORD>(e.wheelDelta);
            }

            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        case MacroEventType::KeyDown:
        case MacroEventType::KeyUp: {
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(e.vkCode);
            input.ki.wScan = static_cast<WORD>(e.scanCode);
            DWORD flags = e.extended ? KEYEVENTF_EXTENDEDKEY : 0;
            if (e.type == MacroEventType::KeyUp) flags |= KEYEVENTF_KEYUP;
            input.ki.dwFlags = flags;
            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        case MacroEventType::KeyPress: {
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(e.vkCode);
            input.ki.wScan = static_cast<WORD>(e.scanCode);
            const DWORD baseFlags = e.extended ? KEYEVENTF_EXTENDEDKEY : 0;
            input.ki.dwFlags = baseFlags;
            SendInput(1, &input, sizeof(INPUT));
            INPUT up = input;
            up.ki.dwFlags = baseFlags | KEYEVENTF_KEYUP;
            SendInput(1, &up, sizeof(INPUT));
            break;
        }
    }
}
