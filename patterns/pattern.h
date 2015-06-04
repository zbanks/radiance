#ifndef __PATTERN_H
#define __PATTERN_H

#include "core/slot.h"

void pattern_init();
void pattern_del();

extern int n_patterns;
extern pattern_t** patterns;

extern pattern_t pat_full;
extern pattern_t pat_wave;
extern pattern_t pat_bubble;
extern pattern_t pat_strobe;

#endif
