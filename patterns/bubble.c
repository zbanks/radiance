
#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

// --------- Pattern: Bubble -----------

typedef struct
{
    color_t color;
    float r;
    float rho;
    float cx;
    float cy;
} pat_bubble_state_t;

enum pat_bubble_param_names {
    BUBBLE_COLOR,
    BUBBLE_R,
    BUBBLE_RHO,
    BUBBLE_CX,
    BUBBLE_CY,

    N_BUBBLE_PARAMS
};

parameter_t pat_bubble_params[N_BUBBLE_PARAMS] = {
    [BUBBLE_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [BUBBLE_R] = {
        .name = "r",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [BUBBLE_RHO] = {
        .name = "\\rho",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [BUBBLE_CX] = {
        .name = "cx",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [BUBBLE_CY] = {
        .name = "cy",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void pat_bubble_init(pat_state_pt state)
{
    pat_bubble_state_t * bubble_state = (pat_bubble_state_t*)state;
    bubble_state->color = (color_t) {0., 0., 0., 0.};
    bubble_state->r = 0.;
    bubble_state->cx = 0.;
    bubble_state->cy = 0.;
}

void pat_bubble_update(slot_t* slot, long t)
{
    UNUSED(t);
    pat_bubble_state_t* state = (pat_bubble_state_t*)slot->state;
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[BUBBLE_COLOR]));
    state->r = param_state_get(&slot->param_states[BUBBLE_R]);
    state->rho = param_state_get(&slot->param_states[BUBBLE_RHO]) * 1.3 + 0.3;
    state->cx = param_state_get(&slot->param_states[BUBBLE_CX]) * 2 - 1.0;
    state->cy = param_state_get(&slot->param_states[BUBBLE_CY]) * 2 - 1.0;
}

int pat_bubble_event(slot_t* slot, struct pat_event event, float event_data){
    if(isnan(event_data)) return 0;
    switch(event.source){
        case PATSRC_MOUSE_X:
            param_state_setq(&slot->param_states[BUBBLE_CX], (event_data + 1.0) / 2);
        break;
        case PATSRC_MOUSE_Y:
            param_state_setq(&slot->param_states[BUBBLE_CY], (event_data + 1.0) / 2);
        break;
        default: return 0;
    }
    return 1;
}

color_t pat_bubble_pixel(const pat_state_pt pat_state_p, float x, float y)
{
    float d;
    const pat_bubble_state_t* state = (const pat_bubble_state_t*)pat_state_p;
    color_t result = state->color;

    d = sqrt(pow(state->cx - x, 2) + pow(state->cy - y, 2)) / state->r;
    
    if(d < 1.0)
        result.a = pow(1.0 - pow(d, state->rho), 1.0 / state->rho);
    else
        result.a = 0.0;
    return result;
}

pattern_t pat_bubble = {
    .render = &pat_bubble_pixel,
    .init = &pat_bubble_init,
    .update = &pat_bubble_update,
    .event = &pat_bubble_event,
    .n_params = N_BUBBLE_PARAMS,
    .parameters = pat_bubble_params,
    .name = "Bubble",
    .state_size = sizeof(pat_bubble_state_t),
};
