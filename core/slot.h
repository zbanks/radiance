#ifndef __SLOT_H
#define __SLOT_H

#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "core/time.h"
#include "util/color.h"
#include "hits/hit.h"

struct slot;
struct hit;
struct pattern;

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();
typedef void (*pat_update_fn_pt)(struct slot* slot, mbeat_t t);
typedef void (*pat_prevclick_fn_pt)(struct slot* slot, float x, float y);
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
    union {
        struct pattern * pattern;
        struct hit * hit;
    };
    void* state;
    param_state_t alpha;
    param_state_t * param_states;
} slot_t;

extern int n_slots;
extern int n_hit_slots;

extern slot_t slots[];
extern slot_t hit_slots[];

void update_patterns(mbeat_t t);
void update_hits(mbeat_t t);

color_t render_composite(float x, float y);

void pat_load(slot_t* slot, struct pattern * pattern);
void pat_unload(slot_t* slot);

void hit_load(slot_t* slot, struct hit * hit);
void hit_unload(slot_t* slot);

extern SDL_mutex* patterns_updating;

#endif
