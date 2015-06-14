
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


// --------- Pattern: Strobe -----------

typedef struct
{
    color_t color;
    struct freq_state freq_state;
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
        .val_to_str = power_zero_quantize_parameter_label,
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
    freq_init(&state->freq_state, 0.5, 1);
    state->last_t = 0;
    state->a = 0.;
    state->hit_dir = 0;
    state->hit_state = 0.;
    return state;
}

void pat_strobe_del(pat_state_pt state)
{
    free(state);
}

void pat_strobe_update(slot_t* slot, mbeat_t t){
    pat_strobe_state_t* state = (pat_strobe_state_t*)slot->state;
    freq_update(&state->freq_state, t, param_state_get(&slot->param_states[STROBE_FREQ]));

    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[STROBE_COLOR]));

    float a = 0;
    float phase = state->freq_state.phase;
    if(phase > (1.0 - param_state_get(&slot->param_states[STROBE_ATTACK]) / 2.)){
        a = (1.0 - phase) / (param_state_get(&slot->param_states[STROBE_ATTACK]) / 2.);
        a = 1.0 - a;
    }else if(phase < param_state_get(&slot->param_states[STROBE_DECAY]) / 2.){
        a = (phase) / (param_state_get(&slot->param_states[STROBE_DECAY]) / 2.);
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


int pat_strobe_event(slot_t* slot, struct pat_event event, float event_data){
    pat_strobe_state_t* state = (pat_strobe_state_t*)slot->state;
    if(event.source == PATSRC_MOUSE_X || event.source == PATSRC_MOUSE_Y)
        event_data = 1.;
    switch(event.event){
        case PATEV_START:
            state->hit_dir = event_data;
            state->hit_state = 0;
        break;
        case PATEV_END:
            state->hit_dir = -state->hit_dir;
        break;
        default: return 0;
    }
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
