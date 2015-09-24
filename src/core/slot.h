#ifndef __SLOT_H
#define __SLOT_H

#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "patterns/pattern.h"
#include "core/time.h"
#include "util/color.h"

#define N_MAX_PARAMS 8

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
    param_state_t param_states[N_MAX_PARAMS];
    pat_state_pt ui_state;
    int solo;
    int mute;
} slot_t;

extern int n_slots;

extern slot_t slots[];

void update_patterns(mbeat_t t);

void render_composite_frame(state_source_t src, float * x, float * y, size_t n, color_t * out);

void pat_load(slot_t* slot, struct pattern * pattern);
void pat_unload(slot_t* slot);
void update_ui();

void slots_init();
void slots_del();

extern SDL_mutex* patterns_updating;

extern struct param_state global_alpha_state;

#endif
