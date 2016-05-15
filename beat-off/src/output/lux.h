#ifndef __OUTPUT_LUX_H__
#define __OUTPUT_LUX_H__

#include "output/slice.h"

int output_lux_init();
int output_lux_enumerate(output_strip_t * strips, int n_strips);
int output_lux_push(output_strip_t * strip, unsigned char * frame, int length);
void output_lux_del();

#endif
