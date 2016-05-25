#pragma once
#include <stdint.h>

extern struct time_master {
    long wall_ms;
    double beat_frac;
    uint8_t beat_index;
} time_master;

struct time_source {
    const char * name;
    double (*update)(struct time_source * source, long wall_ms);
    void (*destroy)(struct time_source * source);
};

int time_master_init();
void time_master_term();
int time_master_register_source(struct time_source * source);
void time_master_update();
