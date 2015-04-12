#ifndef __FILTER_H
#define __FILTER_H

#include "core/parameter.h"
#include "filters/audio.h"

struct filter;

typedef void * filter_state_pt;
typedef void (*filter_init_fn_pt)(struct filter * filter);
typedef void (*filter_update_fn_pt)(struct filter * filter, float t, chunk_t chunk);
typedef void (*filter_del_fn_pt)(struct filter * filter);

typedef struct filter {
    // Input parameters
    int n_params;
    parameter_t* parameters;
    param_state_t* param_states;

    // Output value
    param_output_t output;

    // UI Elements
    char * name;

    // State
    filter_state_pt state;

    // Function Calls
    filter_init_fn_pt init;
    filter_update_fn_pt update;
    filter_del_fn_pt del;
} filter_t;

extern int n_filtered_chunks;
extern int n_filters;
extern filter_t filters[];
extern filter_t beat_filter;

void update_filters(float t, chunk_t chunk);
#endif
