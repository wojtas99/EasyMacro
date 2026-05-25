#ifndef EASYMACRO_MACROPLAYER_H
#define EASYMACRO_MACROPLAYER_H

#include <QThread>
#include <QVector>
#include <atomic>
#include "MacroEvent.h"

class MacroPlayer : public QThread {
    Q_OBJECT
public:
    explicit MacroPlayer(QObject *parent = nullptr);
    ~MacroPlayer() override;

    void configure(const QVector<MacroEvent> &events, int repeatCount, bool infinite, bool instant = false);
    void requestStop();

signals:
    void playbackStarted();
    void playbackFinished();
    void iterationChanged(int current, int total);

protected:
    void run() override;

private:
    void playEvent(const MacroEvent &e);
    bool interruptibleSleep(quint32 ms);

    QVector<MacroEvent> m_events;
    int m_repeatCount = 1;
    bool m_infinite = false;
    bool m_instant = false;
    std::atomic<bool> m_stop{false};
};

#endif
