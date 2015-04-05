#include <math.h>
#include "signal.h"
#include "pattern.h"

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

const parameter_t inp_lfo_parameters[] = {
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

    signal->param_values = malloc(sizeof(float *) * signal->n_params);
    for(int i = 0; i < signal->n_params; i++){
        signal->param_values[i] = pval_new(signal->parameters[i].default_val, signal->parameters);
    }

    signal->value = pval_new(signal->default_val, signal);
}

void inp_lfo_update(signal_t * signal, float t){
    inp_lfo_state_t * state = (inp_lfo_state_t *) signal->state;
    state->phase += (t - state->last_t) * signal->param_values[1]->v;
    state->last_t = t;
    state->type = quantize_parameter(osc_quant_labels, signal->param_values[0]->v);
    signal->value->v = osc_fn_gen(state->type, state->phase) * signal->param_values[2]->v
                      + (1.0 - signal->param_values[2]->v) * signal->param_values[3]->v;
}

void inp_lfo_del(signal_t * signal){
    for(int i = 0; i < signal->n_params; i++){
        pval_free(signal->param_values[i], signal->parameters);
    }
    pval_free(signal->value, signal);
    free(signal->param_values);
    free(signal->state);
}

const int n_signals = N_SIGNALS;
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
