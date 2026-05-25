#ifndef EASYMACRO_SETTINGS_H
#define EASYMACRO_SETTINGS_H

class Settings {
public:
    Settings();

    int runHotkeyVk() const { return m_runVk; }
    int stopHotkeyVk() const { return m_stopVk; }
    int recordHotkeyVk() const { return m_recordVk; }
    int repeatCount() const { return m_repeatCount; }
    bool repeatInfinite() const { return m_infinite; }

    void setRunHotkeyVk(int vk) { m_runVk = vk; }
    void setStopHotkeyVk(int vk) { m_stopVk = vk; }
    void setRecordHotkeyVk(int vk) { m_recordVk = vk; }
    void setRepeatCount(int count) { m_repeatCount = count; }
    void setRepeatInfinite(bool infinite) { m_infinite = infinite; }

    void load();
    void save() const;

private:
    int m_runVk = 0x70;
    int m_stopVk = 0x71;
    int m_recordVk = 0x72;
    int m_repeatCount = 1;
    bool m_infinite = false;
};

#endif
