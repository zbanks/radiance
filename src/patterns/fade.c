#include <math.h>

#include "patterns/pattern.h"
#include "core/slot.h"
#include "util/color.h"
#include "util/math.h"
#include "util/signal.h"

// --------- Pattern: Fade -----------

static const char name[] = "Fade";

typedef struct
{
    color_t color;
    float color_phase;
    struct freq_state freq_state;
    mbeat_t last_event_start;
} state_t;

enum param_names {
    FREQ,
    DELTA,
    COLOR,

    N_PARAMS
};

static const parameter_t params[N_PARAMS] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [FREQ] = {
        .name = "Frequency",
        .default_val = 0.5,
        .val_to_str = power_zero_quantize_parameter_label,
    },
    [DELTA] = {
        .name = "Color Delta",
        .default_val = 0.25,
        .val_to_str = float_to_string,
    },
};

static void init(state_t* state)
{
    state->color = (color_t) {0.0, 0.0, 0.0, 0.0};
    state->color_phase = 0.;
    state->last_event_start = 0;
    freq_init(&state->freq_state, 0.5, 1);
}

static void update(slot_t* slot, mbeat_t t)
{
    state_t* state = (state_t*)slot->state;
    int n_beats = freq_update(&state->freq_state, t, param_state_get(&slot->param_states[FREQ]));

    if(n_beats && (t - state->last_event_start > 1000)){
        state->color_phase += n_beats * param_state_get(&slot->param_states[DELTA]);
        state->color_phase = fmod(state->color_phase, 1.0);
    }

    float x = param_state_get(&slot->param_states[COLOR]);
    x += state->color_phase;
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, fmod(x, 1.0));
}

static void command(slot_t* slot, pat_command_t cmd)
{
    state_t* state = (state_t*)slot->state;
    switch(cmd.status)
    {
        case STATUS_START:
            state->color_phase += param_state_get(&slot->param_states[DELTA]);
            float x = param_state_get(&slot->param_states[COLOR]) + state->color_phase;
            struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
            state->color = colormap_color(cm, fmod(x, 1.0));
            state->last_event_start = state->freq_state.last_t;
            break;
        case STATUS_CHANGE:
        case STATUS_STOP:
            break;
    }
}

static color_t render(const state_t* restrict state, float x, float y)
{
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

pattern_t pat_fade = MAKE_PATTERN;
