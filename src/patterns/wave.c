#include <math.h>

#include "core/slot.h"
#include "patterns/pattern.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"
#include "util/signal.h"

// --------- Pattern: Wave -----------

static const char name[] = "Wave";

typedef struct
{
    color_t color;
    struct freq_state freq_state;
    enum osc_type type;
    mbeat_t last_t;
    float kx;
    float ky;
    float rho;
} state_t;

enum param_names {
    OMEGA,
    K_MAG,
    TYPE,
    K_ANGLE,
    RHO,
    COLOR,

    N_PARAMS
};

static const parameter_t params[N_PARAMS] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [TYPE] = {
        .name = "Wave Type",
        .default_val = 0.0,
        .val_to_str = osc_quantize_parameter_label,
    },
    [OMEGA] = {
        .name = "\\omega",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [K_MAG] = {
        .name = "|k|",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [K_ANGLE] = {
        .name = "<)k",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [RHO] = {
        .name = "\\rho",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

static void init(state_t* state)
{
    state->color = (color_t) {0.0, 0.0, 0.0, 0.0};
    freq_init(&state->freq_state, 0.5, 0);
    state->type = OSC_SINE;
    state->last_t = 0;
    state->kx = 1.0;
    state->ky = 1.0;
    state->rho = 0.5;
}

static void update(slot_t* slot, mbeat_t t)
{
    float k_mag;
    float k_ang;

    state_t* state = (state_t*)slot->state;
    struct colormap * cm = slot->colormap ? slot->colormap : cm_global;
    state->color = colormap_color(cm, param_state_get(&slot->param_states[COLOR]));
    state->type = quantize_parameter(osc_quant_labels, param_state_get(&slot->param_states[TYPE]));

    freq_update(&state->freq_state, t, param_state_get(&slot->param_states[OMEGA]));
    state->last_t = t;

    k_mag = param_state_get(&slot->param_states[K_MAG]) * 2 + 0.2;
    k_ang = param_state_get(&slot->param_states[K_ANGLE]) * 2 * M_PI;
    state->kx = COS(k_ang) * k_mag;
    state->ky = SIN(k_ang) * k_mag;
    state->rho = exp(param_state_get(&slot->param_states[RHO]) * 2 * logf(0.5 - 0.1)) + 0.1;
}

static void command(slot_t* slot, pat_command_t cmd)
{
}

static color_t render(const state_t* restrict state, float x, float y)
{
    color_t result = state->color;
    result.a = osc_fn_gen(state->type, state->freq_state.phase + y * state->ky + x * state->kx);
    result.a = powf(result.a, state->rho);
    return result;
}

pattern_t pat_wave = MAKE_PATTERN;
