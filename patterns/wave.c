
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

// --------- Pattern: Wave -----------

typedef struct
{
    color_t color;
    struct freq_state freq_state;
    enum osc_type type;
    mbeat_t last_t;
    float kx;
    float ky;
} pat_wave_state_t;

enum pat_wave_param_names {
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
        .val_to_str = power_quantize_parameter_label,
    },
    [WAVE_K_MAG] = {
        .name = "|k|",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [WAVE_K_ANGLE] = {
        .name = "<)k",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

pat_state_pt pat_wave_init()
{
    pat_wave_state_t * state = malloc(sizeof(pat_wave_state_t));
    state->color = (color_t) {0.0, 0.0, 0.0, 0.0};
    freq_init(&state->freq_state, 0.5, 0);
    state->type = OSC_SINE;
    state->last_t = 0;
    state->kx = 1.0;
    state->ky = 1.0;
    return state;
}

void pat_wave_del(pat_state_pt state)
{
    free(state);
}

void pat_wave_update(slot_t* slot, mbeat_t t)
{
    float k_mag;
    float k_ang;

    pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    state->color = param_to_color(param_state_get(&slot->param_states[WAVE_COLOR]));
    state->type = quantize_parameter(osc_quant_labels, param_state_get(&slot->param_states[WAVE_TYPE]));

    freq_update(&state->freq_state, t, param_state_get(&slot->param_states[WAVE_OMEGA]));
    state->last_t = t;

    k_mag = param_state_get(&slot->param_states[WAVE_K_MAG]) * 2 + 0.2;
    k_ang = param_state_get(&slot->param_states[WAVE_K_ANGLE]) * 2 * M_PI;
    state->kx = COS(k_ang) * k_mag;
    state->ky = SIN(k_ang) * k_mag;
}

void pat_wave_prevclick(slot_t * slot, float x, float y){
    //pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    /* I don't know how I feel about resetting the state yet...
    state->phase = 0.;
    state->last_t = fmod(state->last_t, 16.0);
    */
    param_state_setq(&slot->param_states[WAVE_K_MAG], sqrt(pow(x, 2) + pow(y, 2)) / sqrt(2.0));
    param_state_setq(&slot->param_states[WAVE_K_ANGLE], (atan2(y, x) / (2.0 * M_PI)) + 0.5);
}

int pat_wave_event(slot_t* slot, enum pat_event event, float event_data){
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

color_t pat_wave_pixel(slot_t* slot, float x, float y)
{
    pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    color_t result = state->color;
    result.a = osc_fn_gen(state->type, state->freq_state.phase + y * state->ky + x * state->kx);
    return result;
}

pattern_t pat_wave = {
    .render = &pat_wave_pixel,
    .init = &pat_wave_init,
    .del = &pat_wave_del,
    .update = &pat_wave_update,
    .event = &pat_wave_event,
    .n_params = N_WAVE_PARAMS,
    .parameters = pat_wave_params,
    .name = "Wave",
};
