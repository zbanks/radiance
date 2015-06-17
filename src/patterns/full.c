#include <math.h>

#include "core/slot.h"
#include "patterns/pattern.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

// --------- Pattern: Full -----------

static const char name[] = "Full";

enum param_names {
    VALUE,
    COLOR,

    N_PARAMS
};

typedef struct
{
    color_t color;
} state_t;

static const parameter_t params[N_PARAMS] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [VALUE] = {
        .name = "Value",
        .default_val = 1.0,
        .val_to_str = float_to_string,
    },
};

static void init(state_t* state)
{
    state->color = (color_t) {.r = 0., .g = 0., .b = 0., .a = 0.};
}

static void update(slot_t* slot, long t)
{
    state_t* state = (state_t*)slot->state;
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));
    float v = param_state_get(&slot->param_states[VALUE]);
    state->color.r *= v;
    state->color.g *= v;
    state->color.b *= v;
}

static color_t render(const state_t* restrict state, float x, float y)
{
    return state->color;
}

static void command(slot_t* slot, pat_command_t cmd)
{
    switch(cmd.index)
    {
        case 0:
            param_state_setq(&slot->param_states[COLOR], cmd.value);
            break;
        case 1:
            param_state_setq(&slot->param_states[VALUE], cmd.value);
            break;
    }
}

pattern_t pat_full = MAKE_PATTERN;
