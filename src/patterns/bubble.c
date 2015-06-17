#include <math.h>

#include "patterns/pattern.h"
#include "core/slot.h"
#include "util/color.h"
#include "util/math.h"

// --------- Pattern: Bubble -----------

typedef struct
{
    color_t color;
    float r;
    float rho;
    float cx;
    float cy;
} state_t;

enum param_names {
    R,
    RHO,
    COLOR,
    CX,
    CY,

    N_PARAMS
};

static parameter_t params[N_PARAMS] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [R] = {
        .name = "r",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [RHO] = {
        .name = "\\rho",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [CX] = {
        .name = "cx",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [CY] = {
        .name = "cy",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

static void init(state_t* state)
{
    state->color = (color_t) {0., 0., 0., 0.};
    state->r = 0.;
    state->cx = 0.;
    state->cy = 0.;
}

static void update(slot_t* slot, long t)
{
    UNUSED(t);
    state_t* state = (state_t*)slot->state;
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));
    state->r = param_state_get(&slot->param_states[R]);
    state->rho = param_state_get(&slot->param_states[RHO]) * 1.3 + 0.3;
    state->cx = param_state_get(&slot->param_states[CX]) * 2 - 1.0;
    state->cy = param_state_get(&slot->param_states[CY]) * 2 - 1.0;
}

static int event(slot_t* slot, struct pat_event ev, float event_data)
{
    switch(ev.source)
    {
        case PATSRC_MOUSE_X:
            param_state_setq(&slot->param_states[CX], (event_data + 1.0) / 2);
            break;
        case PATSRC_MOUSE_Y:
            param_state_setq(&slot->param_states[CY], (event_data + 1.0) / 2);
            break;
        default:
            return 0;
    }
    return 1;
}

static color_t render(const state_t* state, float x, float y)
{
    float d;
    color_t result = state->color;

    d = sqrt(pow(state->cx - x, 2) + pow(state->cy - y, 2)) / state->r;
    
    if(d < 1.0)
        result.a = pow(1.0 - pow(d, state->rho), 1.0 / state->rho);
    else
        result.a = 0.0;
    return result;
}

pattern_t pat_bubble = {
    .render = (pat_render_fn_pt)&render,
    .init = (pat_init_fn_pt)&init,
    .update = &update,
    .event = &event,
    .n_params = N_PARAMS,
    .parameters = params,
    .state_size = sizeof(state_t),
    .name = "Bubble",
};
