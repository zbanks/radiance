#include "core/slot.h"
#include "core/err.h"
#include "patterns/pattern.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"

extern pattern_t pat_dyn;

#define N_PATTERNS 1
pattern_t * patterns[N_PATTERNS] = {&pat_dyn};
int n_patterns = N_PATTERNS;


// --------- Pattern: Full -----------

enum pat_full_param_names {
    FULL_COLOR,
    FULL_VALUE,

    N_FULL_PARAMS
};

static parameter_t pat_full_params[] = {
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

static pat_state_pt pat_full_init()
{
    color_t * color = malloc(sizeof(color_t));
    *color = (color_t) {.r = 0., .g = 0., .b = 0., .a = 0.};
    return color;
}

static void pat_full_del(pat_state_pt state)
{
    free(state);
}

static void pat_full_update(slot_t* slot, long t)
{
    color_t* color = (color_t*)slot->state;
    *color = param_to_color(param_state_get(&slot->param_states[FULL_COLOR]));
    float v = param_state_get(&slot->param_states[FULL_VALUE]);
    color->r *= v;
    color->g *= v;
    color->b *= v;
}

static void pat_full_prevclick(slot_t * slot, float x, float y){
}

static color_t pat_full_pixel(slot_t* slot, float x, float y)
{
    return *(color_t*)slot->state;
}

pattern_t pat_dyn = {
    .render = &pat_full_pixel,
    .init = &pat_full_init,
    .del = &pat_full_del,
    .update = &pat_full_update,
    .prevclick = &pat_full_prevclick,
    .n_params = N_FULL_PARAMS,
    .parameters = pat_full_params,
    .name = "DYNAMIC",
};
