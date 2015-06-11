#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

#define N_PATTERNS 9
pattern_t * default_patterns[N_PATTERNS] = {&pat_full, &pat_wave, &pat_bubble, &pat_strobe, &pat_mstrobe, &pat_fade, &pat_swipe, &pat_rainbow, &pat_sparkle};
pattern_t ** patterns;
int n_patterns = N_PATTERNS;

void pattern_init(){
    patterns = malloc(n_patterns * sizeof(pattern_t *));
    if(!patterns) FAIL("Unable to malloc patterns array\n");
    memcpy(patterns, default_patterns, n_patterns * sizeof(pattern_t *));
}

void pattern_del(){
    free(patterns);
}

