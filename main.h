#ifndef __MAIN_H
#define __MAIN_H

#include "pattern/deck.h"
#include "pattern/crossfader.h"
#include "ui/render.h"

#define N_DECKS 2
extern struct deck deck[N_DECKS];
extern struct crossfader crossfader;
extern struct render render;
extern double time;

extern double audio_hi;
extern double audio_mid;
extern double audio_low;

#endif
