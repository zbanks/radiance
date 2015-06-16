#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

// --------- Pattern: Template -----------

typedef struct {
    int value;
} state_t;

enum param_names {
    VALUE,

    N_PARAMS
};

static parameter_t params[] = {
    [VALUE] = {
        .name = "Value",
        .default_val = 1.0,
        .val_to_str = float_to_string,
    },
};

static void init(pat_state_pt pat_state_p) {
    state_t* state = (state_t*)pat_state_p;
    // TEMPLATE: Cast state to a more useful form and
    // initialize state members here
    UNUSED(state);
}

static void update(slot_t* slot, mbeat_t t) {
    state_t * state = (state_t *) slot->state;
    UNUSED(t);
    UNUSED(state);
    // TEMPLATE: Modify state based on t & parameters
}

static color_t pixel(const pat_state_pt pat_state_p, float x, float y) {
    const state_t * state = (const state_t*)pat_state_p;
    color_t output = {.r = 0., .g = 0., .b = 0., .a = 0.};
    UNUSED(x);
    UNUSED(y);
    UNUSED(state);
    // TEMPLATE: Compute color at (x, y) from state
    return output;
}

static int event(slot_t* slot, struct pat_event ev, float event_data){
    state_t * state = (state_t *) slot->state;
    if(isnan(event_data)) return 0;
    UNUSED(ev);
    UNUSED(state);
    // TEMPLATE: Handle click/MIDI event
    return 0;
}

pattern_t pat_template = {
    .render = &pixel,
    .init = &init,
    .update = &update,
    .event = &event,
    .n_params = N_PARAMS,
    .parameters = params,
    .name = "Template",
};
