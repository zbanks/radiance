#ifndef __SIGGEN_H__
#define __SIGGEN_H__

#include "core/parameter.h"
#include "core/time.h"

enum osc_type {
    OSC_SINE,
    OSC_TRIANGLE,
    OSC_SQUARE,
    OSC_SAWTOOTH_R,
    OSC_SAWTOOTH_F,
};

extern quant_labels_t osc_quant_labels;
float osc_fn_gen(enum osc_type type, float phase);
void osc_quantize_parameter_label(float val, char * buf, int n);

#endif
