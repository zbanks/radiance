#ifndef __TIMEBASE_H
#define __TIMEBASE_H

#include "core/audio.h"

void timebase_init();
void timebase_del();
void timebase_update(chunk_pt chunk);
long timebase_get(); 
float timebase_get_bpm(); 
void timebase_tap();

#endif
