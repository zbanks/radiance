#include <time.h>
#include <math.h>

#include "util/config.h"
#include "util/err.h"
#include "util/math.h"

#include "time/timebase.h"

#define N_MAX_TIME_SOURCES 8

struct time_master time_master;
double phase = 0.0;

// Maximum size of `time_master.beat_index`. 16 to track 4/4 beats+bars
//static uint8_t master_beat_denominator = 16;

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
            if (fabs(r - d) > 1e6) FAIL("x=%lf, d=%lf, e=%lf, r=%lf", x, d, e, r);
        }
    }
}

// Time master
int time_init() {
    memset(&time_master, 0, sizeof time_master);
    time_master.bpm = 140;
    error_wrap_test();
    return 0;
}

void time_term() {
    return;
}

void time_update(enum time_source source, enum time_source_event event, double event_arg) {
    struct timespec tv = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &tv) != 0) {
        ERROR_P("clock_gettime failed");
        return;
    }

    // Convert to milliseconds
    long time_ms = tv.tv_sec * 1000 + tv.tv_nsec / (1000 * 1000);

    long delta_time_ms = time_ms - time_master.wall_ms;
    if (time_master.wall_ms == 0) delta_time_ms = 0;
    (void) delta_time_ms;

    switch (event) {
    case TIME_SOURCE_EVENT_BAR:
        break; //TODO
    case TIME_SOURCE_EVENT_BPM:
        time_master.bpm = event_arg;
        break;
    case TIME_SOURCE_EVENT_BEAT:
        ;
        double ms_until_event = event_arg;
        double master_beat_per_ms = time_master.bpm / MINUTES_PER_MILLISECOND;
        char status = '!';
        (void) status;
        if (event_arg == 0) {
            time_master.beat_frac = 0;
            time_master.beat_index++;
        } else if (event_arg < 0) { // How long ago was the last beat
            time_master.beat_frac = -ms_until_event * master_beat_per_ms;
            status = '<';
        } else {
            time_master.beat_frac = MAX(time_master.beat_frac, 1.0 - ms_until_event * master_beat_per_ms);
            status = '>';
        }
#ifdef WHAT_IS_GOING_ON
        INFO("Beat: %c %0.1f %u %0.3f", status, time_master.bpm, time_master.beat_index, time_master.beat_frac);
#endif
        break;
    case TIME_SOURCE_EVENT_NONE:
    default:
        break;
    }

#ifdef MATH_IS_HARD
    // Convert bpm to beat/ms
    double master_beat_per_ms = time_master.bpm / MINUTES_PER_MILLISECOND;
    phase += delta_time_ms * master_beat_per_ms;

    double phase_est_at_event = phase + master_beat_per_ms * ms_until_event;
    double error = error_wrap(phase_est_at_event, 0.0);
    time_master.bpm += error * 0.5;

    static int n_errors = 0;
    static double total_error = 0;
    n_errors++;
    total_error += fabs(error);

    // Update time_master
    long phase_idx = (long) phase;
    if (phase > 1.0) {
        phase -= phase_idx;
        INFO("Predicted %ld beat(s) at bpm=%lf error=%lf", phase_idx, time_master.bpm, total_error / n_errors);
        n_errors = 0;
        total_error = 0;
    } else {
        phase_idx = 0;
    }

    time_master.beat_frac = phase;
    time_master.beat_index += phase_idx;
#endif

    time_master.wall_ms = time_ms;
}
