#include <math.h>

#include "patterns/pattern.h"
#include "signals/signal.h"
#include "util/siggen.h"
#include "util/math.h"
#include "ui/graph.h"
#include "core/time.h"

#define N_SIGNALS 5
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

typedef struct
{
    float phase;
    mbeat_t last_t;
    float freq;
    enum osc_type type;
} inp_lfo_state_t;

enum inp_lfo_param_names {
    LFO_TYPE,
    LFO_FREQ,
    LFO_AMP,
    LFO_OFFSET,

    N_LFO_PARAMS
};

parameter_t inp_lfo_parameters[N_LFO_PARAMS] = {
    [LFO_TYPE] = {
        .name = "Type",
        .default_val = 0,
        .val_to_str = osc_quantize_parameter_label,
    },
    [LFO_FREQ] = {
        .name = "Freq",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [LFO_AMP] = {
        .name = "Amp",
        .default_val = 1.0,
        .val_to_str = float_to_string,
    },
    [LFO_OFFSET] = {
        .name = "Offset",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void inp_lfo_init(signal_t * signal){
    inp_lfo_state_t * state = signal->state = malloc(sizeof(inp_lfo_state_t));
    if(!signal->state) return;

    state->phase = 0.;
    state->last_t = 0;
    state->freq = 1.0;
    state->type = OSC_SINE;


    signal->param_states = malloc(sizeof(param_state_t) * signal->n_params);
    if(!signal->param_states){
        free(signal->state);
        signal->state = 0;
        return;
    }

    for(int i = 0; i < signal->n_params; i++){
        param_state_init(&signal->param_states[i], signal->parameters[i].default_val);
    }
    //signal->output = malloc(sizeof(param_output_t));
}

void inp_lfo_update(signal_t * signal, mbeat_t t){
    inp_lfo_state_t * state = (inp_lfo_state_t *) signal->state;
    if(!state) return;

    float new_freq = 1.0 / power_quantize_parameter(signal->param_states[LFO_FREQ].value);
#define BMOD(t, f) (t % B2MB(f))
    if(new_freq != state->freq){
        if((BMOD(t, new_freq) < BMOD(state->last_t, new_freq)) && (BMOD(t, state->freq) < BMOD(state->last_t, state->freq))){
            // Update with old phase up until zero crossing
            state->phase += (state->freq - MB2B(BMOD(state->last_t, state->freq))) / state->freq;
            // Update with new phase past zero crossing
            state->last_t += (state->freq - MB2B(BMOD(state->last_t, state->freq)));
            state->freq = new_freq;
        }
    }

    state->phase += MB2B(t - state->last_t) / state->freq; 
    state->phase = fmod(state->phase, 1.0); // Prevent losing float resolution
    state->last_t = t;
    state->type = quantize_parameter(osc_quant_labels, signal->param_states[LFO_TYPE].value);
    param_output_set(&signal->output, osc_fn_gen(state->type, state->phase) * signal->param_states[LFO_AMP].value
                                      + (1.0 - signal->param_states[LFO_AMP].value) * signal->param_states[LFO_OFFSET].value);
}

void inp_lfo_del(signal_t * signal){
    if(!signal->state) return;

    param_output_free(&signal->output);
    free(signal->param_states);
    signal->param_states = 0;
    free(signal->state);
    signal->state = 0;
}

typedef struct
{
    float value;
    mbeat_t last_t;
} inp_lpf_state_t;

enum inp_lpf_param_names {
    LPF_INPUT,
    LPF_ALPHA,
    LPF_BETA,

    N_LPF_PARAMS
};

parameter_t inp_lpf_parameters[N_LPF_PARAMS] = {
    [LPF_INPUT] = {
        .name = "Input",
        .default_val = 0,
        .val_to_str = float_to_string,
    },
    [LPF_ALPHA] = {
        .name = "Rise",
        .default_val = 0.05,
        .val_to_str = float_to_string,
    },
    [LPF_BETA] = {
        .name = "Fall",
        .default_val = 0.05,
        .val_to_str = float_to_string,
    },
};

void inp_lpf_init(signal_t * signal){
    inp_lpf_state_t * state = signal->state = malloc(sizeof(inp_lpf_state_t));
    if(!signal->state) return;

    state->value = 0.;
    state->last_t = 0;

    signal->param_states = malloc(sizeof(param_state_t) * signal->n_params);
    if(!signal->param_states){
        free(signal->state);
        signal->state = 0;
        return;
    }

    for(int i = 0; i < signal->n_params; i++){
        param_state_init(&signal->param_states[i], signal->parameters[i].default_val);
    }
}

void inp_lpf_update(signal_t * signal, mbeat_t t){
    inp_lpf_state_t * state = (inp_lpf_state_t *) signal->state;
    if(!state) return;
    if(!state->last_t) state->last_t = t;
    float x = signal->param_states[LPF_INPUT].value;
    float a = signal->param_states[LPF_ALPHA].value;
    float b = signal->param_states[LPF_BETA].value;
    if(state->last_t + 10 < t){
        a = 1.;
        b = 1.;
    }else{
        state->last_t += 10;
    }

    if(x > state->value)
        state->value = a * x + (1. - a) * state->value;
    else
        state->value = b * x + (1. - b) * state->value;

    param_output_set(&signal->output, state->value);
}

void inp_lpf_del(signal_t * signal){
    if(!signal->state) return;

    param_output_free(&signal->output);
    free(signal->param_states);
    signal->param_states = 0;
    free(signal->state);
    signal->state = 0;
}

typedef struct
{
    float min;
    float max;
    mbeat_t last_t;
} inp_agc_state_t;

enum inp_agc_param_names {
    AGC_INPUT,
    AGC_MAX,
    AGC_MIN,
    AGC_DECAY,

    N_AGC_PARAMS
};

parameter_t inp_agc_parameters[N_AGC_PARAMS] = {
    [AGC_INPUT] = {
        .name = "Input",
        .default_val = 0,
        .val_to_str = float_to_string,
    },
    [AGC_MAX] = {
        .name = "Max (1 -> x)",
        .default_val = 1.,
        .val_to_str = float_to_string,
    },
    [AGC_MIN] = {
        .name = "Min (0 -> x)",
        .default_val = 0.,
        .val_to_str = float_to_string,
    },
    [AGC_DECAY] = {
        .name = "AGC Decay",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void inp_agc_init(signal_t * signal){
    inp_agc_state_t * state = signal->state = malloc(sizeof(inp_lpf_state_t));
    if(!signal->state) return;

    state->min = 0.;
    state->max = 1.;

    signal->param_states = malloc(sizeof(param_state_t) * signal->n_params);
    if(!signal->param_states){
        free(signal->state);
        return;
    }

    for(int i = 0; i < signal->n_params; i++){
        param_state_init(&signal->param_states[i], signal->parameters[i].default_val);
    }
}

void inp_agc_update(signal_t * signal, mbeat_t t){
    if(!signal->state) return;
    inp_agc_state_t * state = (inp_agc_state_t *) signal->state;

    float x = signal->param_states[AGC_INPUT].value;
    float min = signal->param_states[AGC_MIN].value;
    float max = signal->param_states[AGC_MAX].value;
    float a = signal->param_states[AGC_DECAY].value;
    float y;
    if(state->last_t + 10 < t){
        a = 1.;
    }else{
        state->last_t += 10;
    }

    if(a < 1e-4){
        state->max = 1.;
        state->min = 0.;
    }else{
        a = a * 0.01 + 0.99;
        state->max = MAX(state->max * a, x);
        state->min = MIN(state->max - (state->max - state->min) * a, x);
        x = (x - state->min) / (state->max - state->min);
    }

    if(min < max){
        y = x * (max - min) + min;
    }else{
        y = (1. - x) * (min - max) + max;
    }
    param_output_set(&signal->output, y);
}

void inp_agc_del(signal_t * signal){
    if(!signal->state) return;
    param_output_free(&signal->output);
    free(signal->param_states);
    signal->param_states = 0;
    free(signal->state);
    signal->state = 0; 
}

int n_signals = N_SIGNALS;
signal_t signals[N_SIGNALS] = {
    {
        .name = "LFO 1",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = N_LFO_PARAMS,
        .parameters = inp_lfo_parameters,
        .color = {1.0, 0.0, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {255, 0, 0, 255},
            .label_color = {255, 0, 0, 255},
            .label = "LFO"
        },
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LFO 2",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = N_LFO_PARAMS,
        .parameters = inp_lfo_parameters,
        .color = {0.9, 0.3, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {220, 100, 0, 255},
            .label_color = {220, 100, 0, 255},
            .label = "LFO"
        },
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LPF 1",
        .type = SIGNAL_LPF,
        .default_val = 0.5,
        .n_params = N_LPF_PARAMS,
        .parameters = inp_lpf_parameters,
        .color = {0.9, 0.9, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {240, 240, 0, 255},
            .label_color = {240, 240, 0, 255},
            .label = "LPF"
        },
        .init = inp_lpf_init,
        .update = inp_lpf_update,
        .del = inp_lpf_del,
    },
    {
        .name = "AGC 1",
        .type = SIGNAL_AGC,
        .default_val = 0.5,
        .n_params = N_AGC_PARAMS,
        .parameters = inp_agc_parameters,
        .color = {0.1, 0.9, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {25, 240, 0, 255},
            .label_color = {25, 240, 0, 255},
            .label = "LPF"
        },
        .init = inp_agc_init,
        .update = inp_agc_update,
        .del = inp_agc_del,
    },
    {
        .name = "AGC 2",
        .type = SIGNAL_AGC,
        .default_val = 0.5,
        .n_params = N_AGC_PARAMS,
        .parameters = inp_agc_parameters,
        .color = {0.0, 0.8, 0.8, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {0, 220, 220, 255},
            .label_color = {0, 220, 220, 255},
            .label = "LPF"
        },
        .init = inp_agc_init,
        .update = inp_agc_update,
        .del = inp_agc_del,
    },
};

void update_signals(mbeat_t t) {
    for(int i=0; i < n_signals; i++)
    {
        signals[i].update(&signals[i], t);
    }
}


void signal_start(){
    for(int i = 0; i < n_signals; i++){
        signals[i].init(&signals[i]);

        graph_create_signal(&signals[i].graph_state);
    }
}

void signal_stop(){
    for(int i = 0; i < n_signals; i++){
        signals[i].del(&signals[i]);

        graph_remove(&signals[i].graph_state);
    }
}
