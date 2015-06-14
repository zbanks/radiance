#ifndef __SLOT_H
#define __SLOT_H

#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "patterns/pattern.h"
#include "core/time.h"
#include "util/color.h"

struct slot;
struct pattern;

typedef enum
{
    STATE_SOURCE_OUTPUT,
    STATE_SOURCE_UI,
} state_source_t;

typedef struct slot
{
    struct pattern * pattern;
    pat_state_pt state;
    struct colormap * colormap;
    param_state_t alpha;
    param_state_t * param_states;
    pat_state_pt ui_state;
} slot_t;

extern int n_slots;

extern slot_t slots[];

void update_patterns(mbeat_t t);

void render_composite_frame(state_source_t src, float * x, float * y, size_t n, color_t * out);
//color_t render_composite(float x, float y);

void pat_load(slot_t* slot, struct pattern * pattern);
void pat_unload(slot_t* slot);
void update_ui();

extern SDL_mutex* patterns_updating;

#endif
