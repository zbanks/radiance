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

// --------- Pattern: Swipe -----------

static const char name[] = "Swipe";

enum swipe_state {
    SWIPE_OFF = 0,
    SWIPE_GROWING,
    SWIPE_SHRINKING,
};

struct swipe {
    int cmd_index;
    int cmd_value;
    double kx; // Wave vector, normalized s.t. sqrt(kx * kx + ky * ky) == 1.0
    double ky;
    double ox; // Origin
    double oy; 
    double length; // Length so far, it grows as the button is held
    double alpha;  // Alpha
    enum swipe_state state; // State: growing, shrinking, off
};

#define N_SWIPE_BUFFER 20

typedef struct {
    double freq;
    mbeat_t last_t;
    color_t color;
    double sharp;
    struct swipe swipe_buffer[N_SWIPE_BUFFER];
} state_t;

enum param_names {
    SPEED,
    SHARP,
    COLOR,

    N_PARAMS
};

static parameter_t params[] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [SPEED] = {
        .name = "Speed",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [SHARP] = {
        .name = "Sharpness",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

static void init(state_t* state)
{
    memset(state, 0, sizeof(state_t));
}

static void update(slot_t* slot, mbeat_t t) {
    state_t * state = (state_t *) slot->state;
    if(state->last_t == 0) state->last_t = t;

    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));
    state->freq = power_quantize_parameter(param_state_get(&slot->param_states[SPEED]));
    state->sharp = param_state_get(&slot->param_states[SHARP]);

    double delta = state->freq * 2 * MB2B(t - state->last_t);

    for(int i = 0; i < N_SWIPE_BUFFER; i++){
        struct swipe * swipe = &state->swipe_buffer[i];
        switch(swipe->state){
            case SWIPE_GROWING:
                swipe->length += delta;
            break;
            case SWIPE_SHRINKING:
                swipe->ox += delta * swipe->kx;
                swipe->oy += delta * swipe->ky;
                if(! ((swipe->ox > -2 && swipe->ox < 2) && (swipe->oy > -2 && swipe->oy < 2))){
                    // Remove swipe, clear it, and put it back in the inactive pile
                    memset(swipe, 0, sizeof(struct swipe));
                }
            break;
            case SWIPE_OFF:
            default:
            break;
        }
    }
    state->last_t = t;
}

static color_t render(const state_t* state, float x, float y)
{
    color_t output = state->color;
    float a = 0.;

    for(int i = 0; i < N_SWIPE_BUFFER; i++){
        const struct swipe * swipe = &state->swipe_buffer[i];
        if(swipe->state != SWIPE_OFF){
            double fuzz = MIN(swipe->length / 2, state->sharp);
            double v = (x - swipe->ox) * swipe->kx + (y - swipe->oy) * swipe->ky;
            if(v > swipe->length)
                a += 0;
            else if(v > (swipe->length - fuzz))
                a += swipe->alpha * (swipe->length - v) / fuzz;
            else if(v > 0)
                a += swipe->alpha;
            else if(v > -fuzz)
                a += swipe->alpha * (fuzz + v) / fuzz;
            else
                a += 0;
        }
    }
    output.a = MIN(a, 1.0);
    return output;
}

static void command(slot_t* slot, pat_command_t cmd)
{
    state_t * state = (state_t *) slot->state;
    struct swipe * swipe;
    int i;

    switch(cmd.status)
    {
        case STATUS_START:
            for(i = 0; i < N_SWIPE_BUFFER - 1; i++)
            {
                if(state->swipe_buffer[i].state == SWIPE_OFF) break;
            }
            swipe = &state->swipe_buffer[i];
            swipe->cmd_index = cmd.index;
            swipe->cmd_value = cmd.value;
            swipe->length = 0;
            swipe->state = SWIPE_GROWING;
            swipe->alpha = cmd.value;
            switch(cmd.index)
            {
                case 0:
                    swipe->ox = 1;
                    swipe->oy = 0;
                    swipe->kx = -1;
                    swipe->ky = 0;
                    break;
                case 1:
                    swipe->ox = -1;
                    swipe->oy = 0;
                    swipe->kx = 1;
                    swipe->ky = 0;
                    break;
                case 2:
                    swipe->ox = -1;
                    swipe->oy = -1;
                    swipe->kx = .707;
                    swipe->ky = .707;
                    swipe->alpha = 1.;
                    break;
            }
            break;
        case STATUS_STOP:
            for(i = 0; i < N_SWIPE_BUFFER - 1; i++)
            {
                if(state->swipe_buffer[i].state == SWIPE_GROWING && state->swipe_buffer[i].cmd_index == cmd.index)
                {
                    state->swipe_buffer[i].state = SWIPE_SHRINKING;
                }
            }
            break;
        case STATUS_CHANGE:
            break;
    }
}

pattern_t pat_swipe = MAKE_PATTERN;
