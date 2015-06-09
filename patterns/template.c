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

static pat_state_pt init() {
    state_t * state = malloc(sizeof(state_t));
    // TEMPLATE: Initialize state members here
    return state;
}

static void del(pat_state_pt state) {
    free(state);
    // TEMPLATE: Free any additional regions malloc'd in init
}

static void update(slot_t* slot, long t) {
    state_t * state = (state_t *) slot->state;
    UNUSED(t);
    UNUSED(state);
    // TEMPLATE: Modify state based on t & parameters
}

static color_t pixel(slot_t* slot, float x, float y) {
    state_t * state = (state_t *) slot->state;
    color_t output = {.r = 0., .g = 0., .b = 0., .a = 0.};
    UNUSED(x);
    UNUSED(y);
    UNUSED(state);
    // TEMPLATE: Compute color at (x, y) from state
    return output;
}

static int event(slot_t* slot, enum pat_event event, float event_data){
    state_t * state = (state_t *) slot->state;
    if(isnan(event_data)) return 0;
    UNUSED(event);
    UNUSED(state);
    // TEMPLATE: Handle click/MIDI event
    return 0;
}

pattern_t pat_template = {
    .render = &pixel,
    .init = &init,
    .del = &del,
    .update = &update,
    .event = &event,
    .n_params = N_PARAMS,
    .parameters = params,
    .name = "Template",
};
