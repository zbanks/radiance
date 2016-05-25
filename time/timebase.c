#include <time.h>
#include <math.h>

#include "util/config.h"
#include "util/err.h"

#include "time/timebase.h"

#define N_MAX_TIME_SOURCES 8

struct time_master time_master;
// Maximum size of `master.beat_index`. 16 to track 4/4 beats+bars
uint8_t master_beat_denominator;

// TODO: linked list or something... does it actually matter though?
size_t n_time_sources = 0;
struct time_source * time_sources[N_MAX_TIME_SOURCES];
double source_phases[N_MAX_TIME_SOURCES];
size_t primary_source = 0;

int time_master_init() {
    return 0;
}

void time_master_term() {
    for (size_t i = 0; i < n_time_sources; i++) {
        time_sources[i]->destroy(time_sources[i]);
    }
}

int time_master_register_source(struct time_source * source) {
    time_sources[n_time_sources] = source;
    return n_time_sources++;
}

void time_master_update() {
    struct timespec tv = {0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &tv) != 0) {
        ERROR_P("clock_gettime failed");
        return;
    }

    // Convert to milliseconds
    long time_ms = tv.tv_sec * 1000 + tv.tv_nsec / (1000 * 1000);

    long delta_time_ms = time_ms - time_master.wall_ms;
    if (time_master.wall_ms == 0) delta_time_ms = 0;

    // Update all time sources
    for (size_t i = 0; i < n_time_sources; i++) {
        double delta_phase = time_sources[i]->update(time_sources[i], delta_time_ms);

        // error condition
        if (isnan(delta_phase)) continue;

        source_phases[i] = fmod(source_phases[i] + delta_phase, master_beat_denominator);
    }

    // Use primary source to update master
    double phase = source_phases[primary_source];
    time_master.wall_ms = time_ms;
    time_master.beat_frac = fmod(phase, 1.0);
    time_master.beat_index = floor(phase);
}


