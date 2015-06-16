#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

// --------- Pattern: Speckle -----------

#define BUCKET_SIZE 1031 // This should be prime

typedef struct {
    color_t color;
    mbeat_t last_t;
    float pixels[BUCKET_SIZE];
} state_t;

enum param_names {
    GEN,
    DECAY,
    COLOR,

    N_PARAMS
};

static parameter_t params[N_PARAMS] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
        .val_to_str = NULL,
    },
    [GEN] = {
        .name = "Fill Fraction",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [DECAY] = {
        .name = "Decay Time",
        .default_val = 0.3,
        .val_to_str = power_quantize_parameter_label,
    },
};

static void init(pat_state_pt pat_state_p) {
    state_t * state = (state_t*)pat_state_p;
    memset(state, 0, sizeof(state_t));
}

static void update(slot_t* slot, mbeat_t t) {
    state_t * state = (state_t *) slot->state;
    float dt = MB2B(t - state->last_t);
    float decay = dt * power_quantize_parameter(param_state_get(&slot->param_states[DECAY]));
    for(int i = 0; i < BUCKET_SIZE; i++){
    }
    // I can't explain where the extra factor of 0.1 comes from? But it seems to be right
    int gen = RAND_MAX * param_state_get(&slot->param_states[GEN]) * (decay / dt) * 0.1;
    for(int i = 0; i < BUCKET_SIZE; i++){
        state->pixels[i] -= decay;
        if(state->pixels[i] < 0)
            state->pixels[i] = 0;

        if(rand() < gen)
            state->pixels[i] = 1.;
    }
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));
    state->last_t = t;
}

static color_t pixel(const pat_state_pt pat_state_p, float x, float y)
{
    const state_t * state = (const state_t*)pat_state_p;
    uint64_t hash;
    memcpy(&hash, &x, 4);
    memcpy(((uint32_t *) &hash)+1, &y, 4);
    size_t b = hash % BUCKET_SIZE;

    color_t output = state->color;
    output.a = state->pixels[b];

    return output;
}

static int event(slot_t* slot, struct pat_event event, float event_data){
    state_t * state = (state_t *) slot->state;
    if(isnan(event_data)) return 0;
    UNUSED(event);
    UNUSED(state);
    // TEMPLATE: Handle click/MIDI event
    return 0;
}

pattern_t pat_sparkle = {
    .render = &pixel,
    .init = &init,
    .update = &update,
    .event = &event,
    .n_params = N_PARAMS,
    .parameters = params,
    .name = "Sparkle",
    .state_size = sizeof(state_t),
};
