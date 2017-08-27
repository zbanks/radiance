#include "Timebase.h"
#include <cmath>
#include <stdexcept>

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
            if (std::abs(r - d) > 1e6) throw std::runtime_error("Precision error"); //"x=%lf, d=%lf, e=%lf, r=%lf", x, d, e, r);
        }
    }
}

Timebase::Timebase()
    : m_wall_ns(0)
    , m_beatFrac(0)
    , m_beatIndex(0)
    , m_bpm(140)
{
    error_wrap_test();
    m_timer.start();
}

void Timebase::update(enum Timebase::TimeSource source, enum Timebase::TimeSourceEvent event, double eventArg) {
    // Convert to milliseconds
    auto time_ns = m_timer.nsecsElapsed();

    QMutexLocker locker(&m_timeLock);
    auto deltaTime_ns = time_ns - m_wall_ns;
    if (m_wall_ns == 0)
        deltaTime_ns = 0;

    switch (event) {
    case TimeSourceEventBar:
        break; //TODO
    case TimeSourceEventBPM:
        m_bpm = eventArg;
        break;
    case TimeSourceEventBeat:
        {
            if (source == TimeSourceDiscrete) {
                m_beatFrac = fmod(eventArg, 1.0);
                m_beatIndex = ((int) eventArg)  % 1024;
            } else {
                auto nsUntilEvent = eventArg * 1000 * 1000;
                auto masterBeatPerNs = m_bpm / NS_PER_MINUTE;
                char status = '!';
                if(eventArg == 0) {
                    m_beatFrac = 0;
                    m_beatIndex = (m_beatIndex + 1) % 1024;
                } else if (eventArg < 0) { // How long ago was the last beat
                    m_beatFrac = -nsUntilEvent * masterBeatPerNs;
                    status = '<';
                } else {
                    m_beatFrac = std::max<double>(m_beatFrac, 1.0 - nsUntilEvent * masterBeatPerNs);
                    status = '>';
                }
            }
        }
        break;
    case TimeSourceEventNone:
    default:
        break;
    }

    m_wall_ns = time_ns;
}
double Timebase::wallTime() const {
    QMutexLocker locker(&m_timeLock);
    return m_wall_ns * 1e-9;
}
double Timebase::beat() const {
    QMutexLocker locker(&m_timeLock);
    return m_beatFrac + m_beatIndex;
}

int Timebase::beatIndex() const {
    QMutexLocker locker(&m_timeLock);
    return m_beatIndex;
}
