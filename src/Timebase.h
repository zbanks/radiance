#pragma once

#include <QElapsedTimer>
#include <QMutex>

class Timebase {
public:
    const double MS_PER_MINUTE = 60 * 1000;

    enum TimeSource {
        TimeSourceAudio,
        TimeSourceUser,
        TimeSourceClock,
        //TimeSourceNetwork,
    };

    enum TimeSourceEvent {
        TimeSourceEventNone,
        TimeSourceEventBeat,
        TimeSourceEventBar,
        TimeSourceEventBPM,
    };

    Timebase();
    void update(enum TimeSource source, enum TimeSourceEvent, double eventArg);
    double beat();
    int beatIndex();

protected:
    long m_wall_ms;
    double m_beatFrac;
    int m_beatIndex;
    double m_bpm;
    QElapsedTimer m_timer;
    QMutex m_timeLock;
};
