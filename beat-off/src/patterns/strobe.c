#include <math.h>

#include "core/slot.h"
#include "patterns/pattern.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"
#include "util/signal.h"

// --------- Pattern: Strobe -----------

const char name[] = "Strobe";

typedef struct
{
    color_t color;
    struct freq_state freq_state;
    mbeat_t last_t;
    float hit_dir;
    float hit_state;
    float a;
} state_t;

enum param_names {
    FREQ,
    ATTACK,
    DECAY,
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
    [ATTACK] = {
        .name = "Attack",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [DECAY] = {
        .name = "Decay",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void init(state_t* state)
{
    state->color = (color_t) {0.0, 0.0, 0.0, 0.0};
    freq_init(&state->freq_state, 0.5, 1);
    state->last_t = 0;
    state->a = 0.;
    state->hit_dir = 0;
    state->hit_state = 0.;
}

static void update(slot_t* slot, mbeat_t t)
{
    state_t* state = (state_t*)slot->state;
    freq_update(&state->freq_state, t, param_state_get(&slot->param_states[FREQ]));

    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));

    float a = 0;
    float phase = state->freq_state.phase;
    if(phase > (1.0 - param_state_get(&slot->param_states[ATTACK]) / 2.)){
        a = (1.0 - phase) / (param_state_get(&slot->param_states[ATTACK]) / 2.);
        a = 1.0 - a;
    }else if(phase < param_state_get(&slot->param_states[DECAY]) / 2.){
        a = (phase) / (param_state_get(&slot->param_states[DECAY]) / 2.);
        a = 1.0 - a;
    }else{
        a = 0.;
    }

    if(state->hit_dir != 0){
        if(state->hit_dir > 0){
            // Attack
            state->hit_state += MB2B(t - state->last_t) / (param_state_get(&slot->param_states[ATTACK]) / 4. + 0.05);
            if(state->hit_state >= 1.){
                state->hit_state = 1.;
            }
        }else{
            state->hit_state -= MB2B(t - state->last_t) / (param_state_get(&slot->param_states[DECAY]) /4. + 0.05);
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

static void command(slot_t* slot, struct pat_command cmd)
{
    state_t* state = (state_t*)slot->state;
    switch(cmd.status)
    {
        case STATUS_START:
            state->hit_dir = cmd.value;
            state->hit_state = 0;
            break;
        case STATUS_STOP:
            state->hit_dir = -fabs(state->hit_dir);
            break;
        case STATUS_CHANGE:
            break;
    }
}

static color_t render(const state_t* restrict state, float x, float y)
{
    color_t result = state->color;
    result.a = state->a;
    return result;
}

pattern_t pat_strobe = MAKE_PATTERN;
