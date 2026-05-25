#ifndef EASYMACRO_MACROEVENT_H
#define EASYMACRO_MACROEVENT_H

#include <QtGlobal>
#include <QJsonObject>

enum class MacroEventType {
    MouseMove = 0,
    MouseDown = 1,
    MouseUp = 2,
    MouseWheel = 3,
    KeyDown = 4,
    KeyUp = 5,
    KeyPress = 6
};

enum class MouseButton {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3
};

struct MacroEvent {
    MacroEventType type = MacroEventType::MouseMove;
    quint32 delayMs = 0;
    int x = 0;
    int y = 0;
    MouseButton button = MouseButton::None;
    int wheelDelta = 0;
    quint32 vkCode = 0;
    quint32 scanCode = 0;
    bool extended = false;
    quint32 randomDelayMaxMs = 0;

    QJsonObject toJson() const;
    static MacroEvent fromJson(const QJsonObject &obj);
};

#endif
