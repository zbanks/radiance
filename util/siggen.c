#include <math.h>

#include "util/siggen.h"
#include "core/parameter.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

quant_labels_t osc_quant_labels = {
    "Sine",
    "Triangle",
    "Sawtooth",
    "Square",
    LABELS_END
};

float osc_fn_gen(enum osc_type type, float phase){
    switch(type){
        case OSC_SINE:
        default:
            return (sin(phase * 2 * M_PI) + 1.0) / 2.0;
        case OSC_TRIANGLE:
            return fabs(fmod(phase + 16.0, 1.0) - 0.5) * 2;
        case OSC_SAWTOOTH:
            return fmod(phase + 16.0, 1.0);
        case OSC_SQUARE:
            return (fmod(phase + 16.0, 1.0) > 0.5 ? 1.0 : 0.0);
    }
}

