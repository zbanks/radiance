#ifndef __MAIN_H
#define __MAIN_H

#include "util/common.h"

#include "pattern/deck.h"
#include "pattern/crossfader.h"
#include "ui/render.h"

#define N_DECKS 4
NOT_CXX
extern struct deck deck[N_DECKS];
extern struct crossfader crossfader;
extern struct render render;
extern double current_time;

extern double audio_hi;
extern double audio_mid;
extern double audio_low;
extern double audio_level;
CXX_OK
#endif
