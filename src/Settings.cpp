#include "Settings.h"

#include <QSettings>

Settings::Settings() {
    load();
}

void Settings::load() {
    QSettings store;
    m_runVk = store.value("hotkeys/run", 0x70).toInt();
    m_stopVk = store.value("hotkeys/stop", 0x71).toInt();
    m_recordVk = store.value("hotkeys/record", 0x72).toInt();
    m_repeatCount = store.value("playback/repeatCount", 1).toInt();
    m_infinite = store.value("playback/infinite", false).toBool();
    m_instantSpeed = store.value("playback/instantSpeed", false).toBool();
    if (m_repeatCount < 1) {
        m_repeatCount = 1;
    }
}

void Settings::save() const {
    QSettings store;
    store.setValue("hotkeys/run", m_runVk);
    store.setValue("hotkeys/stop", m_stopVk);
    store.setValue("hotkeys/record", m_recordVk);
    store.setValue("playback/repeatCount", m_repeatCount);
    store.setValue("playback/infinite", m_infinite);
    store.setValue("playback/instantSpeed", m_instantSpeed);
}
