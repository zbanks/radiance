#ifndef __PATTERN_H
#define __PATTERN_H

#include "slot.h"

extern const int n_patterns;
extern const pattern_t* patterns[];

extern const pattern_t pat_full;
extern const pattern_t pat_wave;
extern const pattern_t pat_bubble;

#define LABELS_END 0
typedef char *quant_labels_t[];
int quantize_parameter(quant_labels_t l, float p);

enum osc_type {
    OSC_SINE,
    OSC_TRIANGLE,
    OSC_SAWTOOTH,
    OSC_SQUARE,
};

extern const quant_labels_t osc_quant_labels;
float osc_fn_gen(enum osc_type type, float phase);

#endif
