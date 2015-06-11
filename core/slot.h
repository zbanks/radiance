#ifndef __SLOT_H
#define __SLOT_H

#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "patterns/pattern.h"
#include "core/time.h"
#include "util/color.h"

struct slot;
struct pattern;

typedef struct slot
{
    struct pattern * pattern;
    void* state;
    struct colormap * colormap;
    param_state_t alpha;
    param_state_t * param_states;
} slot_t;

extern int n_slots;

extern slot_t slots[];

void update_patterns(mbeat_t t);

void render_composite_frame(slot_t * fslots, float * x, float * y, size_t n, color_t * out);
color_t render_composite(float x, float y);

void pat_load(slot_t* slot, struct pattern * pattern);
void pat_unload(slot_t* slot);

extern SDL_mutex* patterns_updating;

#endif
