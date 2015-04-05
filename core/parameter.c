#include <math.h>

#include "core/parameter.h"

int quantize_parameter(quant_labels_t l, float p){ 
    int i = 0;
    int r;
    while(l[++i]);
    r = floor(p * i);
    if(r >= i) r = i - 1;
    return r;
}

