#ifndef __DECK_H
#define __DECK_H

#include "deck/pattern.h"

struct deck {
    struct render_target * preview_rt; // Array with one entry per pattern slot
    struct render_target *** output_device_rt; // Array with one entry per pattern slot
    struct pattern ** pattern; // Array with one entry per pattern slot (NULL if not filled)
};

void deck_init(void* output_device_arrangement);

void deck_term();

#endif
