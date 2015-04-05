#ifndef __SLOT_H
#define __SLOT_H

#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "util/color.h"

struct slot;

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();
typedef void (*pat_update_fn_pt)(struct slot* slot, float t);
typedef void (*pat_prevclick_fn_pt)(struct slot*, float x, float y);
typedef color_t (*pat_render_fn_pt)(struct slot* slot, float x, float y);
typedef void (*pat_del_fn_pt)(pat_state_pt state);

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_prevclick_fn_pt prevclick;
    pat_render_fn_pt render;
    pat_del_fn_pt del;
    int n_params;
    parameter_t* parameters;
    char* name;
} pattern_t;

typedef struct slot
{
    pattern_t* pattern;
    void* state;
    float alpha;
    param_state_t ** param_states;
} slot_t;

extern int n_slots;

extern slot_t slots[];

void update_patterns(float t);
color_t render_composite(float x, float y);

void pat_load(slot_t* slot, pattern_t* pattern);
void pat_unload(slot_t* slot);

extern SDL_mutex* patterns_updating;

#endif
