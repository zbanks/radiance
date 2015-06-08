
#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"


// --------- Pattern: Strobe -----------

typedef struct
{
    color_t color;
    float phase;
    float freq;
    mbeat_t last_t;
} pat_strobe_state_t;

enum pat_strobe_param_names {
    STROBE_COLOR,
    STROBE_FREQ,
    STROBE_ATTACK,
    STROBE_DECAY,

    N_STROBE_PARAMS
};

parameter_t pat_strobe_params[N_STROBE_PARAMS] = {
    [STROBE_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [STROBE_FREQ] = {
        .name = "Frequency",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [STROBE_ATTACK] = {
        .name = "Attack",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [STROBE_DECAY] = {
        .name = "Decay",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

pat_state_pt pat_strobe_init()
{
    pat_strobe_state_t * state = malloc(sizeof(pat_strobe_state_t));
    state->color = (color_t) {0.0, 0.0, 0.0, 0.0};
    state->phase = 0.0;
    state->last_t = 0;
    state->freq = 1.0;
    return state;
}

void pat_strobe_del(pat_state_pt state)
{
    free(state);
}

void pat_strobe_update(slot_t* slot, mbeat_t t)
{
    pat_strobe_state_t* state = (pat_strobe_state_t*)slot->state;
    float new_freq = 1.0 / power_quantize_parameter(param_state_get(&slot->param_states[STROBE_FREQ]));
#define BMOD(t, f) (t % B2MB(f))
    if(new_freq != state->freq){
        if((BMOD(t, new_freq) < BMOD(state->last_t, new_freq)) && (BMOD(t, state->freq) < BMOD(state->last_t, state->freq))){
            // Update with old phase up until zero crossing
            state->phase += (state->freq - MB2B(BMOD(state->last_t, state->freq))) / state->freq;
            // Update with new phase past zero crossing
            state->last_t += (state->freq - MB2B(BMOD(state->last_t, state->freq)));
            state->freq = new_freq;
        }
    }

    state->phase += MB2B(t - state->last_t) / state->freq; 
    state->phase = fmod(state->phase, 1.0); // Prevent losing float resolution
    state->last_t = t;
    state->color = param_to_color(param_state_get(&slot->param_states[STROBE_COLOR]));
}

void pat_strobe_prevclick(slot_t * slot, float x, float y){
    UNUSED(slot);
    UNUSED(x);
    UNUSED(y);
    //pat_strobe_state_t* state = (pat_strobe_state_t*)slot->state;
    /* I don't know how I feel about resetting the state yet...
    state->phase = 0.;
    state->last_t = fmod(state->last_t, 16.0);
    */
    //param_state_setq(&slot->param_states[STROBE_K_MAG], sqrt(pow(x, 2) + pow(y, 2)) / sqrt(2.0));
    //param_state_setq(&slot->param_states[STROBE_K_ANGLE], (atan2(y, x) / (2.0 * M_PI)) + 0.5);
}

int pat_strobe_event(slot_t* slot, enum pat_event event, float event_data){
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

color_t pat_strobe_pixel(slot_t* slot, float x, float y)
{
    UNUSED(x);
    UNUSED(y);
    pat_strobe_state_t* state = (pat_strobe_state_t*)slot->state;
    color_t result = state->color;
    float a;
    if(state->phase > (1.0 - param_state_get(&slot->param_states[STROBE_ATTACK]) / 2.)){
        a = (1.0 - state->phase) / (param_state_get(&slot->param_states[STROBE_ATTACK]) / 2.);
        a = 1.0 - a;
    }else if(state->phase < param_state_get(&slot->param_states[STROBE_DECAY]) / 2.){
        a = (state->phase) / (param_state_get(&slot->param_states[STROBE_DECAY]) / 2.);
        a = 1.0 - a;
    }else{
        a = 0.;
    }

    result.a = a;
    return result;
}

pattern_t pat_strobe = {
    .render = &pat_strobe_pixel,
    .init = &pat_strobe_init,
    .del = &pat_strobe_del,
    .update = &pat_strobe_update,
    .event = &pat_strobe_event,
    .n_params = N_STROBE_PARAMS,
    .parameters = pat_strobe_params,
    .name = "Strobe",
};
