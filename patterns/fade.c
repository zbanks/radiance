
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


// --------- Pattern: Fade -----------

typedef struct
{
    color_t color;
    float color_phase;
    struct freq_state freq_state;
} pat_fade_state_t;

enum pat_fade_param_names {
    FADE_COLOR,
    FADE_FREQ,
    FADE_DELTA,

    N_FADE_PARAMS
};

parameter_t pat_fade_params[N_FADE_PARAMS] = {
    [FADE_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [FADE_FREQ] = {
        .name = "Frequency",
        .default_val = 0.5,
        .val_to_str = power_zero_quantize_parameter_label,
    },
    [FADE_DELTA] = {
        .name = "Color Delta",
        .default_val = 0.25,
        .val_to_str = float_to_string,
    },
};

pat_state_pt pat_fade_init()
{
    pat_fade_state_t * state = malloc(sizeof(pat_fade_state_t));
    state->color = (color_t) {0.0, 0.0, 0.0, 0.0};
    state->color_phase = 0.;
    freq_init(&state->freq_state, 0.5, 1);
    return state;
}

void pat_fade_del(pat_state_pt state)
{
    free(state);
}

void pat_fade_update(slot_t* slot, mbeat_t t)
{
    pat_fade_state_t* state = (pat_fade_state_t*)slot->state;
    int n_beats = freq_update(&state->freq_state, t, param_state_get(&slot->param_states[FADE_FREQ]));

    if(n_beats){
        state->color_phase += n_beats * param_state_get(&slot->param_states[FADE_DELTA]);
        state->color_phase = fmod(state->color_phase, 1.0);
    }

    float x = param_state_get(&slot->param_states[FADE_COLOR]);
    x += state->color_phase;
    state->color = colormap_color(&cm_rainbow_equal, fmod(x, 1.0));
}

int pat_fade_event(slot_t* slot, enum pat_event event, float event_data){
    pat_fade_state_t* state = (pat_fade_state_t*)slot->state;
    UNUSED(event_data);
    switch(event){
        case PATEV_MOUSE_DOWN_X:
        case PATEV_M1_NOTE_ON:
        case PATEV_M2_NOTE_ON:
            state->color_phase += param_state_get(&slot->param_states[FADE_DELTA]);
            float x = param_state_get(&slot->param_states[FADE_COLOR]) + state->color_phase;
            state->color = colormap_color(&cm_rainbow_equal, fmod(x, 1.0));
        break;
        default: return 0;
    }
    return 1;
}

color_t pat_fade_pixel(slot_t* slot, float x, float y)
{
    UNUSED(x);
    UNUSED(y);
    pat_fade_state_t* state = (pat_fade_state_t*)slot->state;
    color_t result = state->color;
    /*
    float a;
    if(state->phase > (1.0 - param_state_get(&slot->param_states[FADE_ATTACK]) / 2.)){
        a = (1.0 - state->phase) / (param_state_get(&slot->param_states[FADE_ATTACK]) / 2.);
        a = 1.0 - a;
    }else if(state->phase < param_state_get(&slot->param_states[FADE_DECAY]) / 2.){
        a = (state->phase) / (param_state_get(&slot->param_states[FADE_DECAY]) / 2.);
        a = 1.0 - a;
    }else{
        a = 0.;
    }

    result.a = a;
    */
    return result;
}

pattern_t pat_fade = {
    .render = &pat_fade_pixel,
    .init = &pat_fade_init,
    .del = &pat_fade_del,
    .update = &pat_fade_update,
    .event = &pat_fade_event,
    .n_params = N_FADE_PARAMS,
    .parameters = pat_fade_params,
    .name = "Fade",
};
