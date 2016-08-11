#pragma once
#include "util/common.h"
#include <stdint.h>

#define MINUTES_PER_MILLISECOND (60. * 1000.)
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
