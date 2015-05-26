#ifndef __TIMEBASE_H
#define __TIMEBASE_H

#include "core/audio.h"

enum {
    TB_MANUAL,
    TB_AUTOMATIC,
} timebase_state;

extern int stat_fps;
extern int stat_ops;
extern volatile int timebase_source;

void timebase_init();
void timebase_del();
long timebase_get(); 
float timebase_get_bpm(); 
void timebase_tap(double alpha);
void timebase_align(); 

#endif
