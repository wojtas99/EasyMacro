#include "MacroEvent.h"

QJsonObject MacroEvent::toJson() const {
    QJsonObject obj;
    obj.insert("type", static_cast<int>(type));
    obj.insert("delayMs", static_cast<double>(delayMs));
    obj.insert("x", x);
    obj.insert("y", y);
    obj.insert("button", static_cast<int>(button));
    obj.insert("wheelDelta", wheelDelta);
    obj.insert("vkCode", static_cast<double>(vkCode));
    obj.insert("scanCode", static_cast<double>(scanCode));
    obj.insert("extended", extended);
    obj.insert("randomDelayMaxMs", static_cast<double>(randomDelayMaxMs));
    return obj;
}

MacroEvent MacroEvent::fromJson(const QJsonObject &obj) {
    MacroEvent e;
    e.type = static_cast<MacroEventType>(obj.value("type").toInt());
    e.delayMs = static_cast<quint32>(obj.value("delayMs").toDouble());
    e.x = obj.value("x").toInt();
    e.y = obj.value("y").toInt();
    e.button = static_cast<MouseButton>(obj.value("button").toInt());
    e.wheelDelta = obj.value("wheelDelta").toInt();
    e.vkCode = static_cast<quint32>(obj.value("vkCode").toDouble());
    e.scanCode = static_cast<quint32>(obj.value("scanCode").toDouble());
    e.extended = obj.value("extended").toBool();
    e.randomDelayMaxMs = static_cast<quint32>(obj.value("randomDelayMaxMs").toDouble());
    return e;
}
