#ifndef __VAMP_H
#define __VAMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "filters/filter.h"
#include "core/audio.h"

int vamp_plugin_load(filter_t * filter);
void vamp_plugin_unload(filter_t * filter);
int vamp_plugin_update(filter_t * filter, chunk_pt chunk);

#ifdef __cplusplus
}
#endif

#endif
