#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"
#include "util/signal.h"
#include "util/perlin.h"

// --------- Pattern: Template -----------

static const char name[] = "Perlin";

typedef struct {
    struct colormap* colormap;
    mbeat_t last_mbeat;
    float t;
    float zoom;
} state_t;

enum param_names
{
    SPEED,
    ZOOM,

    N_PARAMS
};

static const parameter_t params[N_PARAMS] = {
    [SPEED] = {
        .name = "Speed",
        .default_val = 0.2,
    },
    [ZOOM] = {
        .name = "Zoom",
        .default_val = 0.5,
    },
};


static void init(state_t* state)
{
    state->colormap = cm_global;
    state->last_mbeat = 0;
    state->t = (float)(rand() / (RAND_MAX / 1000));
    state->zoom = 0;
}

static void update(slot_t* slot, mbeat_t t)
{
    state_t * state = (state_t *) slot->state;

    float v = param_state_get(&slot->param_states[SPEED]) * 2;
    state->zoom = (1 - param_state_get(&slot->param_states[ZOOM])) * 4;

    state->t = fmod(state->t + (float)(t - state->last_mbeat) / 1000 * v, PERLIN_PERIOD);
    state->last_mbeat = t;

    state->colormap = slot->colormap ? slot->colormap : cm_global;
}

static color_t render(const state_t* restrict state, float x, float y)
{
    return colormap_color(state->colormap, perlin3d(x * state->zoom, y * state->zoom, state->t));
}

static void command(slot_t* slot, pat_command_t cmd)
{
}

pattern_t pat_perlin = MAKE_PATTERN;
