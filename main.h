#ifndef __MAIN_H
#define __MAIN_H

#include "pattern/deck.h"
#include "pattern/crossfader.h"

#define N_DECKS 2
extern struct deck deck[N_DECKS];
extern struct crossfader crossfader;
extern double time;

#endif
