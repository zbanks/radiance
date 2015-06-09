
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
    float hit_dir;
    float hit_state;
    float a;
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
    state->a = 0.;
    state->hit_dir = 0;
    state->hit_state = 0.;
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
        float next_freq = (new_freq > state->freq) ? state->freq * 2. : state->freq / 2.;
        if((BMOD(t, next_freq) < BMOD(state->last_t, next_freq)) && (BMOD(t, state->freq) < BMOD(state->last_t, state->freq))){
            // Update with old phase up until zero crossing
            state->phase += (state->freq - MB2B(BMOD(state->last_t, state->freq))) / state->freq;
            // Update with new phase past zero crossing
            state->last_t += (state->freq - MB2B(BMOD(state->last_t, state->freq)));
            state->freq = next_freq;
        }
    }

    state->phase += MB2B(t - state->last_t) / state->freq; 
    state->phase = fmod(state->phase, 1.0); // Prevent losing float resolution
    state->color = param_to_color(param_state_get(&slot->param_states[STROBE_COLOR]));

    float a = 0;
    if(state->phase > (1.0 - param_state_get(&slot->param_states[STROBE_ATTACK]) / 2.)){
        a = (1.0 - state->phase) / (param_state_get(&slot->param_states[STROBE_ATTACK]) / 2.);
        a = 1.0 - a;
    }else if(state->phase < param_state_get(&slot->param_states[STROBE_DECAY]) / 2.){
        a = (state->phase) / (param_state_get(&slot->param_states[STROBE_DECAY]) / 2.);
        a = 1.0 - a;
    }else{
        a = 0.;
    }

    if(state->hit_dir != 0){
        if(state->hit_dir > 0){
            // Attack
            state->hit_state += MB2B(t - state->last_t) / (param_state_get(&slot->param_states[STROBE_ATTACK]) / 4. + 0.05);
            if(state->hit_state >= 1.){
                state->hit_state = 1.;
            }
        }else{
            state->hit_state -= MB2B(t - state->last_t) / (param_state_get(&slot->param_states[STROBE_DECAY]) /4. + 0.05);
            if(state->hit_state <= 0.){
                // Remove hit
                state->hit_state = 0;
                state->hit_dir = 0;
            }
        }
    }

    state->last_t = t;
    state->a = MIN(1.0, fabs(state->hit_dir) * state->hit_state + a);
}


int pat_strobe_event(slot_t* slot, enum pat_event event, float event_data){
    pat_strobe_state_t* state = (pat_strobe_state_t*)slot->state;
    switch(event){
        case PATEV_MOUSE_DOWN_X:
            state->hit_dir = 1.;
            state->hit_state = 0;
        break;
        case PATEV_M1_NOTE_ON:
        case PATEV_M2_NOTE_ON:
            state->hit_dir = event_data;
            state->hit_state = 0;
        break;
        case PATEV_MOUSE_UP_X:
        case PATEV_M1_NOTE_OFF:
        case PATEV_M2_NOTE_OFF:
            state->hit_dir = -state->hit_dir;
        break;
        default: return 0;
    }
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
    result.a = state->a;
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
