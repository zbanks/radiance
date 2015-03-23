#ifndef __SLOT_H
#define __SLOT_H

#include <pthread.h>

typedef struct color
{
    float r;
    float g;
    float b;
    float a;
} color_t;

struct slot;

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();
typedef void (*pat_update_fn_pt)(struct slot* slot, float t);
typedef color_t (*pat_render_fn_pt)(struct slot* slot, float x, float y);
typedef void (*pat_del_fn_pt)(pat_state_pt state);
typedef void (*param_val_to_str_fn_pt)(float val, char* buf, int n);

typedef struct parameter
{
    char* name;
    param_val_to_str_fn_pt val_to_str;
    float default_val;
} parameter_t;

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_render_fn_pt render;
    pat_del_fn_pt del;
    int n_params;
    parameter_t* parameters;
    const char* name;
} pattern_t;

typedef struct slot
{
    const pattern_t* pattern;
    void* state;
    float* param_values;
} slot_t;

extern const int n_slots;

extern slot_t slots[];

void update_patterns(float t);
color_t render_composite(float x, float y);

void pat_load(slot_t* slot, pattern_t* pattern);
void pat_unload(slot_t* slot);

extern pthread_mutex_t patterns_updating;

#endif
