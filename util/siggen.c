#include <math.h>

#include "util/siggen.h"
#include "util/math.h"
#include "core/parameter.h"

quant_labels_t osc_quant_labels = {
    [OSC_SINE] = "Sine",
    [OSC_TRIANGLE] = "Triangle",
    [OSC_SAWTOOTH_R] = "Sawtooth Rising",
    [OSC_SAWTOOTH_F] = "Sawtooth Falling",
    [OSC_SQUARE] = "Square",
    LABELS_END
};

float osc_fn_gen(enum osc_type type, float phase){
    switch(type){
        case OSC_SINE:
        default:
            return (COS(phase * 2 * M_PI) + 1.0) / 2.0;
        case OSC_TRIANGLE:
            return ABS(fmod(phase + 16.0, 1.0) - 0.5) * 2;
        case OSC_SAWTOOTH_R:
            return fmod(phase + 16.0, 1.0);
        case OSC_SAWTOOTH_F:
            return fmod(-phase + 16.0, 1.0);
        case OSC_SQUARE:
            return (fmod(phase + 16.0 + 0.5, 1.0) > 0.5 ? 1.0 : 0.0);
    }
}

void osc_quantize_parameter_label(float val, char * buf, int n){
    int v = quantize_parameter(osc_quant_labels, val);
    strncpy(buf, osc_quant_labels[v], n);
}
