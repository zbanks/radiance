
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

pat_state_pt pat_bubble_init()
{
    pat_bubble_state_t * state = malloc(sizeof(pat_bubble_state_t));
    state->color = (color_t) {0., 0., 0., 0.};
    state->r = 0.;
    state->cx = 0.;
    state->cy = 0.;
    return state;
}

void pat_bubble_del(pat_state_pt state)
{
    free(state);
}

void pat_bubble_update(slot_t* slot, long t)
{
    UNUSED(t);
    pat_bubble_state_t* state = (pat_bubble_state_t*)slot->state;
    state->color = param_to_color(slot->param_states[BUBBLE_COLOR].value);
    state->r = slot->param_states[BUBBLE_R].value;
    state->rho = slot->param_states[BUBBLE_RHO].value * 1.3 + 0.3;
    state->cx = slot->param_states[BUBBLE_CX].value * 2 - 1.0;
    state->cy = slot->param_states[BUBBLE_CY].value * 2 - 1.0;
}

void pat_bubble_prevclick(slot_t * slot, float x, float y){
    // TODO: check that we have control of the param before writing to it
    slot->param_states[BUBBLE_CX].value = (x + 1.0) / 2;
    slot->param_states[BUBBLE_CY].value = (y + 1.0) / 2;
}

int pat_bubble_event(slot_t* slot, enum pat_event event, float event_data){
    UNUSED(slot);
    UNUSED(event);
    UNUSED(event_data);
    /*
    switch(event){
        default:
    }
    */
    return 0;
}

color_t pat_bubble_pixel(slot_t* slot, float x, float y)
{
    float d;
    pat_bubble_state_t* state = (pat_bubble_state_t*)slot->state;
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
    .del = &pat_bubble_del,
    .update = &pat_bubble_update,
    .event = &pat_bubble_event,
    .n_params = N_BUBBLE_PARAMS,
    .parameters = pat_bubble_params,
    .name = "Bubble",
};
