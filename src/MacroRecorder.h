#ifndef EASYMACRO_MACRORECORDER_H
#define EASYMACRO_MACRORECORDER_H

#include <QObject>
#include <QVector>
#include <QElapsedTimer>
#include "MacroEvent.h"

class MacroRecorder : public QObject {
    Q_OBJECT
public:
    explicit MacroRecorder(QObject *parent = nullptr);
    ~MacroRecorder() override;

    bool installHooks();
    void removeHooks();

    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_recording; }

    const QVector<MacroEvent> &events() const { return m_events; }
    void setEvents(const QVector<MacroEvent> &events) { m_events = events; }
    void clear() { m_events.clear(); }
    int eventCount() const { return m_events.size(); }

    void setHotkeys(int runVk, int stopVk, int recordVk);

    bool handleKeyboard(int vk, int scan, bool extended, bool keyUp, bool injected);
    void handleMouse(unsigned int message, int x, int y, int wheelDelta, bool injected);

signals:
    void runHotkeyPressed();
    void stopHotkeyPressed();
    void recordHotkeyPressed();
    void eventRecorded(int totalCount);

private:
    quint32 nextTimestamp();

    void *m_keyboardHook = nullptr;
    void *m_mouseHook = nullptr;
    bool m_recording = false;
    QVector<MacroEvent> m_events;
    QElapsedTimer m_timer;
    qint64 m_lastTimestamp = 0;

    int m_runVk = 0x70;
    int m_stopVk = 0x71;
    int m_recordVk = 0x72;
};

#endif
