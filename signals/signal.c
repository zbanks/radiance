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

parameter_t inp_lfo_parameters[] = {
    {
        .name = "Type",
        .default_val = 0,
    },
    {
        .name = "Freq",
        .default_val = 0.5,
    },
    {
        .name = "Amp",
        .default_val = 1.0,
    },
    {
        .name = "Offset",
        .default_val = 0.5,
    },
};

void inp_lfo_init(signal_t * signal){
    signal->state = malloc(sizeof(inp_lfo_state_t));
    signal->output = malloc(sizeof(param_state_t));
}

void inp_lfo_update(signal_t * signal, float t){
    inp_lfo_state_t * state = (inp_lfo_state_t *) signal->state;
    state->phase += (t - state->last_t) * signal->param_states[1]->value;
    state->last_t = t;
    state->type = quantize_parameter(osc_quant_labels, signal->param_states[0]->value);
    signal->output->value = osc_fn_gen(state->type, state->phase) * signal->param_states[2]->value
                            + (1.0 - signal->param_states[2]->value) * signal->param_states[3]->value;
}

void inp_lfo_del(signal_t * signal){
    free(signal->output);
    free(signal->state);
}

int n_signals = N_SIGNALS;
signal_t signals[N_SIGNALS] = {
    {
        .name = "LFO 1",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = sizeof(inp_lfo_parameters) / sizeof(parameter_t),
        .parameters = inp_lfo_parameters,
        .color = {1.0, 0.0, 0.0, 0.0},
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LFO 2",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = sizeof(inp_lfo_parameters) / sizeof(parameter_t),
        .parameters = inp_lfo_parameters,
        .color = {0.9, 0.3, 0.0, 0.0},
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LFO 3",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = sizeof(inp_lfo_parameters) / sizeof(parameter_t),
        .parameters = inp_lfo_parameters,
        .color = {0.9, 0.9, 0.0, 0.0},
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
    }
}

void signal_stop(){
    for(int i = 0; i < n_signals; i++){
        signals[i].del(&signals[i]);
    }
}
