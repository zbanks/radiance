#include <math.h>

#include "patterns/pattern.h"
#include "signals/signal.h"
#include "util/siggen.h"

#define N_SIGNALS 3
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

typedef struct
{
    float phase;
    float last_t;
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
        .val_to_str = float_to_string,
    },
    [LFO_FREQ] = {
        .name = "Freq",
        .default_val = 0.5,
        .val_to_str = float_to_string,
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
    signal->state = malloc(sizeof(inp_lfo_state_t));
    signal->param_states = malloc(sizeof(param_state_t) * signal->n_params);
    for(int i = 0; i < signal->n_params; i++){
        signal->param_states[i].value = signal->parameters[i].default_val;
        signal->param_states[i].connected_output = 0;
        signal->param_states[i].next_connected_state = 0;
        signal->param_states[i].prev_connected_state = 0;
    }
    //signal->output = malloc(sizeof(param_output_t));
}

void inp_lfo_update(signal_t * signal, float t){
    inp_lfo_state_t * state = (inp_lfo_state_t *) signal->state;
    state->phase += (t - state->last_t) * signal->param_states[LFO_FREQ].value;
    state->phase = fmod(state->phase, 1.0); // Prevent losing float resolution
    state->last_t = t;
    state->type = quantize_parameter(osc_quant_labels, signal->param_states[LFO_TYPE].value);
    param_output_set(&signal->output, osc_fn_gen(state->type, state->phase) * signal->param_states[LFO_AMP].value
                                      + (1.0 - signal->param_states[LFO_AMP].value) * signal->param_states[LFO_OFFSET].value);
}

void inp_lfo_del(signal_t * signal){
    param_output_free(&signal->output);
    free(signal->param_states);
    free(signal->state);
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
            .handle_color = {255, 0, 0},
            .label_color = {255, 0, 0},
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
            .handle_color = {240, 40, 0},
            .label_color = {240, 40, 0},
            .label = "LFO"
        },
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LFO 3",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = N_LFO_PARAMS,
        .parameters = inp_lfo_parameters,
        .color = {0.9, 0.9, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {240, 240, 0},
            .label_color = {240, 240, 0},
            .label = "LFO"
        },
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    }
};

void update_signals(float t) {
    for(int i=0; i < n_signals; i++)
    {
        signals[i].update(&signals[i], t);
    }
}


void signal_start(){
    for(int i = 0; i < n_signals; i++){
        signals[i].init(&signals[i]);

        graph_create(&signals[i].graph_state);
    }
}

void signal_stop(){
    for(int i = 0; i < n_signals; i++){
        signals[i].del(&signals[i]);

        graph_remove(&signals[i].graph_state);
    }
}
