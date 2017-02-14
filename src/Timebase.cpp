#include "Timebase.h"
#include <cmath>
#include <stdexcept>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Utility functions
static double error_wrap(double x_est, double x_obs) {
    // Error of `x_est` from `x_obs` in the space [-1, 1]
    // D(0.1, 0) == -D(0.9, 0) == 0.1
    // D(0.5, 0) == +0.5; D(0.51, 0) == -0.49
    double d = x_est - x_obs + 100;
    return fmod(d + 0.5, 1.0) - 0.5;
}

static void error_wrap_test() {
    for(double x = -0.95; x <= 1.0; x += 0.05) {
        for (double d = -0.99; d <= 1.0; d += 0.01) {
            double e = x + d;
            while(e >= 1.0) e -= 1.0;
            while(e < 0) e += 1.0;
            double r = error_wrap(e, x);
            if (fabs(r - d) > 1e6) throw std::runtime_error("Precision error"); //"x=%lf, d=%lf, e=%lf, r=%lf", x, d, e, r);
        }
    }
}

Timebase::Timebase()
    : m_wall_ms(0)
    , m_beatFrac(0)
    , m_beatIndex(0)
    , m_bpm(140)
{
    error_wrap_test();
    m_timer.start();
}

void Timebase::update(enum Timebase::TimeSource source, enum Timebase::TimeSourceEvent event, double eventArg) {
    // Convert to milliseconds
    long time_ms = m_timer.elapsed();

    QMutexLocker locker(&m_timeLock);
    long deltaTime_ms = time_ms - m_wall_ms;
    if (m_wall_ms == 0) deltaTime_ms = 0;

    switch (event) {
    case TimeSourceEventBar:
        break; //TODO
    case TimeSourceEventBPM:
        m_bpm = eventArg;
        break;
    case TimeSourceEventBeat:
        {
            double msUntilEvent = eventArg;
            double masterBeatPerMs = m_bpm / MS_PER_MINUTE;
            char status = '!';
            if(eventArg == 0) {
                m_beatFrac = 0;
                m_beatIndex = (m_beatIndex + 1) % 1024;
            } else if (eventArg < 0) { // How long ago was the last beat
                m_beatFrac = -msUntilEvent * masterBeatPerMs;
                status = '<';
            } else {
                m_beatFrac = MAX(m_beatFrac, 1.0 - msUntilEvent * masterBeatPerMs);
                status = '>';
            }
        }
        break;
    case TimeSourceEventNone:
    default:
        break;
    }

    m_wall_ms = time_ms;
}

double Timebase::beat() {
    QMutexLocker locker(&m_timeLock);
    return m_beatFrac + m_beatIndex;
}

int Timebase::beatIndex() {
    QMutexLocker locker(&m_timeLock);
    return m_beatIndex;
}
