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

static const char name[] = "PSparkle";

typedef struct {
    color_t c;
    mbeat_t last_mbeat;
    float t;
    float zoom;
    float hardness;
    float density;
} state_t;

enum param_names
{
    SPEED,
    ZOOM,
    DENSITY,
    HARDNESS,
    COLOR,

    N_PARAMS
};

static const parameter_t params[N_PARAMS] = {
    [SPEED] = {
        .name = "Speed",
        .default_val = 0.5,
    },
    [ZOOM] = {
        .name = "Zoom",
        .default_val = 0.5,
    },
    [DENSITY] = {
        .name = "Density",
        .default_val = 0.5,
    },
    [HARDNESS] = {
        .name = "Hardness",
        .default_val = 0.5,
    },
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
};


static void init(state_t* state)
{
    state->c = (color_t){0, 0, 0, 0};
    state->last_mbeat = 0;
    state->t = (float)(rand() / (RAND_MAX / 1000));
    state->zoom = 0;

    state->density = 0;
    state->hardness = 0;
}

static void update(slot_t* slot, mbeat_t t)
{
    state_t * state = (state_t *) slot->state;

    float v = param_state_get(&slot->param_states[SPEED]) * 4;
    state->zoom = (1 - param_state_get(&slot->param_states[ZOOM])) * 6;

    state->density = (1 - param_state_get(&slot->param_states[DENSITY])) * 0.5;
    state->hardness = param_state_get(&slot->param_states[HARDNESS]) * 5;
    state->hardness *= state->hardness;

    state->t = fmod(state->t + (float)(t - state->last_mbeat) / 1000 * v, PERLIN_PERIOD);
    state->last_mbeat = t;

    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->c = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));
}

static color_t render(const state_t* restrict state, float x, float y)
{
    color_t c = state->c;
    c.a = state->hardness * (perlin3d(x * state->zoom, y * state->zoom, state->t) - state->density) + 0.5;
    if(c.a > 1) c.a = 1;
    if(c.a < 0) c.a = 0;
    return c;
}

static void command(slot_t* slot, pat_command_t cmd)
{
}

pattern_t pat_psparkle = MAKE_PATTERN;
