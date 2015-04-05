#include <stdlib.h>
#include <math.h>

#include "core/slot.h"
#include "patterns/pattern.h"
#include "util/color.h"
#include "util/siggen.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

pattern_t* patterns[] = {&pat_full, &pat_wave, &pat_bubble};
int n_patterns = sizeof(patterns) / sizeof(pattern_t);

// --------- Pattern: Full -----------

pat_state_pt pat_full_init()
{
    return malloc(sizeof(color_t));
}

void pat_full_del(pat_state_pt state)
{
    free(state);
}

void pat_full_update(slot_t* slot, float t)
{
    color_t* color = (color_t*)slot->state;
    *color = param_to_color(slot->param_states[0]->value);
}

void pat_full_prevclick(slot_t * slot, float x, float y){

}

color_t pat_full_pixel(slot_t* slot, float x, float y)
{
    return *(color_t*)slot->state;
}

parameter_t pat_full_params[] = {
    {
        .name = "Color",
        .default_val = 0.5,
    },
};

pattern_t pat_full = {
    .render = &pat_full_pixel,
    .init = &pat_full_init,
    .del = &pat_full_del,
    .update = &pat_full_update,
    .prevclick = &pat_full_prevclick,
    .n_params = sizeof(pat_full_params) / sizeof(parameter_t),
    .parameters = pat_full_params,
    .name = "Full",
};

// --------- Pattern: Wave -----------

typedef struct
{
    color_t color;
    float phase;
    enum osc_type type;
    float last_t;
    float kx;
    float ky;
} pat_wave_state_t;

pat_state_pt pat_wave_init()
{
    return malloc(sizeof(pat_wave_state_t));
}

void pat_wave_del(pat_state_pt state)
{
    free(state);
}

void pat_wave_update(slot_t* slot, float t)
{
    float k_mag;
    float k_ang;

    pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    state->color = param_to_color(slot->param_states[0]->value);
    state->type = quantize_parameter(osc_quant_labels, slot->param_states[1]->value);

    state->phase += (t - state->last_t) * slot->param_states[2]->value;
    state->last_t = t;

    k_mag = slot->param_states[3]->value * 2 + 0.2;
    k_ang = slot->param_states[4]->value * 2 * M_PI;
    state->kx = cos(k_ang) * k_mag;
    state->ky = sin(k_ang) * k_mag;
}

void pat_wave_prevclick(slot_t * slot, float x, float y){
    // TODO: check that we have control of the param before writing to it
    slot->param_states[3]->value = sqrt(pow(x, 2) + pow(y, 2)) / sqrt(2.0);
    slot->param_states[4]->value = (atan2(y, x) / (2.0 * M_PI)) + 0.5;
}

color_t pat_wave_pixel(slot_t* slot, float x, float y)
{
    pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    color_t result = state->color;
    result.a = osc_fn_gen(state->type, state->phase + y * state->ky + x * state->kx);
    return result;
}

static enum pat_wave_param_names {
    WAVE_COLOR,
    WAVE_TYPE,
    WAVE_OMEGA,
    WAVE_K_MAG,
    WAVE_K_ANGLE,

    N_WAVE_PARAMS
};

parameter_t pat_wave_params[N_WAVE_PARAMS] = {
    [WAVE_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [WAVE_TYPE] = {
        .name = "Wave Type",
        .default_val = 0.0,
    },
    [WAVE_OMEGA] = {
        .name = "\\omega",
        .default_val = 0.5,
    },
    [WAVE_K_MAG] = {
        .name = "|k|",
        .default_val = 0.5,
    },
    [WAVE_K_ANGLE] = {
        .name = "<)k",
        .default_val = 0.5,
    },
};

pattern_t pat_wave = {
    .render = &pat_wave_pixel,
    .init = &pat_wave_init,
    .del = &pat_wave_del,
    .update = &pat_wave_update,
    .prevclick = &pat_wave_prevclick,
    .n_params = sizeof(pat_wave_params) / sizeof(parameter_t),
    .parameters = pat_wave_params,
    .name = "Wave",
};

// --------- Pattern: Bubble -----------

typedef struct
{
    color_t color;
    float r;
    float rho;
    float cx;
    float cy;
} pat_bubble_state_t;

pat_state_pt pat_bubble_init()
{
    return malloc(sizeof(pat_bubble_state_t));
}

void pat_bubble_del(pat_state_pt state)
{
    free(state);
}

void pat_bubble_update(slot_t* slot, float t)
{
    pat_bubble_state_t* state = (pat_bubble_state_t*)slot->state;
    state->color = param_to_color(slot->param_states[0]->value);
    state->r = slot->param_states[1]->value;
    state->rho = slot->param_states[2]->value * 1.3 + 0.3;
    state->cx = slot->param_states[3]->value * 2 - 1.0;
    state->cy = slot->param_states[4]->value * 2 - 1.0;
}

void pat_bubble_prevclick(slot_t * slot, float x, float y){
    // TODO: check that we have control of the param before writing to it
    slot->param_states[3]->value = (x + 1.0) / 2;
    slot->param_states[4]->value = (y + 1.0) / 2;
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

parameter_t pat_bubble_params[] = {
    {
        .name = "Color",
        .default_val = 0.5,
    },
    {
        .name = "r",
        .default_val = 0.5,
    },
    {
        .name = "\\rho",
        .default_val = 0.5,
    },
    {
        .name = "cx",
        .default_val = 0.5,
    },
    {
        .name = "cy",
        .default_val = 0.5,
    },
};

pattern_t pat_bubble = {
    .render = &pat_bubble_pixel,
    .init = &pat_bubble_init,
    .del = &pat_bubble_del,
    .update = &pat_bubble_update,
    .prevclick = &pat_bubble_prevclick,
    .n_params = sizeof(pat_bubble_params) / sizeof(parameter_t),
    .parameters = pat_bubble_params,
    .name = "Bubble",
};
