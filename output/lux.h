#pragma once
#include "util/common.h"
#ifdef __cplusplus
extern "C" {
#endif
int output_lux_init();
void output_lux_term();

int output_lux_prepare_frame();
int output_lux_sync_frame();


#ifdef __cplusplus
}
#endif
