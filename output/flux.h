#ifndef __OUTPUT_FLUX_H__
#define __OUTPUT_FLUX_H__

#include "output/slice.h"

int output_flux_init();
int output_flux_enumerate(output_strip_t * strips, int n_strips);
int output_flux_push(output_strip_t * strip, unsigned char * frame, int length);
void output_flux_del();

#endif
