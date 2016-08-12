#pragma once
#include "util/common.h"
#include <stdint.h>
#include <x86intrin.h>

#define MILLISECONDS_PER_MINUTE (60. * 1000.)
#define MINUTES_PER_MILLISECOND (1e-3/60)
extern struct time_master {
    long    wall_ms;
    double  beat_frac;
    uint8_t beat_index;
    double  bpm;
} time_master;

enum time_source {
    TIME_SOURCE_AUDIO,
    TIME_SOURCE_USER,
    TIME_SOURCE_CLOCK,
    //TIME_SOURCE_NETWORK,
};

enum time_source_event {
    TIME_SOURCE_EVENT_NONE,
    TIME_SOURCE_EVENT_BEAT,
    TIME_SOURCE_EVENT_BAR,
    TIME_SOURCE_EVENT_BPM,
};

int time_init();
void time_term();
void time_update(time_source source, time_source_event event, double ms_until_event);

double  time_cpu_frequency(void);
double  time_cpu_frequency_inverse(void);
double  time_cpu_time(void);
double  time_cpu_nanosleep_update(int64_t ns);
double  time_cpu_nanosleep_estimate(int64_t ns);
int64_t time_cpu_ref_cycles(void);
