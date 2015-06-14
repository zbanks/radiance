#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

// --------- Pattern: Full -----------

enum pat_full_param_names {
    FULL_COLOR,
    FULL_VALUE,

    N_FULL_PARAMS
};

parameter_t pat_full_params[] = {
    [FULL_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [FULL_VALUE] = {
        .name = "Value",
        .default_val = 1.0,
        .val_to_str = float_to_string,
    },
};

void pat_full_init(pat_state_pt state)
{
    color_t * color = (color_t*)state;
    *color = (color_t) {.r = 0., .g = 0., .b = 0., .a = 0.};
}

void pat_full_update(slot_t* slot, long t)
{
    UNUSED(t);
    color_t* color = (color_t*)slot->state;
    //*color = param_to_color(param_state_get(&slot->param_states[FULL_COLOR]));
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    *color = colormap_color(cm, param_state_get(&slot->param_states[FULL_COLOR]));
    float v = param_state_get(&slot->param_states[FULL_VALUE]);
    color->r *= v;
    color->g *= v;
    color->b *= v;
}

color_t pat_full_pixel(const pat_state_pt pat_state_p, float x, float y)
{
    UNUSED(x);
    UNUSED(y);
    return *(const color_t*)pat_state_p;
}

int pat_full_event(slot_t* slot, struct pat_event event, float event_data){
    if(isnan(event_data)) return 0;
    switch(event.source){
        case PATSRC_MOUSE_X:
        case PATSRC_MIDI_0:
            param_state_setq(&slot->param_states[FULL_COLOR], event_data / 2 + 0.5);
        break;
        case PATSRC_MOUSE_Y:
        case PATSRC_MIDI_1:
            param_state_setq(&slot->param_states[FULL_VALUE], event_data / 2 + 0.5);
        break;
        default: return 0;
    }
    return 1;
}

pattern_t pat_full = {
    .render = &pat_full_pixel,
    .init = &pat_full_init,
    .update = &pat_full_update,
    .event = &pat_full_event,
    .n_params = N_FULL_PARAMS,
    .parameters = pat_full_params,
    .name = "Full",
    .state_size = sizeof(color_t),
};
