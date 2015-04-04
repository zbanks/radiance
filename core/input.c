#include <math.h>
#include "input.h"

#define N_INPUTS 1
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

enum osc_type {
    OSC_SINE,
    OSC_SQUARE,
    OSC_TRIANGLE,
    OSC_SAWTOOTH,
};

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

void inp_lfo_init(input_t * input){
    input->state = malloc(sizeof(inp_lfo_state_t));

    input->param_values = malloc(sizeof(float *) * input->n_params);
    for(int i = 0; i < input->n_params; i++){
        input->param_values[i] = pval_new(input->parameters[i].default_val, input->parameters);
    }

    input->value = pval_new(input->default_val, input);
}

void inp_lfo_update(input_t * input, float t){
    inp_lfo_state_t * state = (inp_lfo_state_t *) input->state;
    state->phase += (t - state->last_t) * input->param_values[1]->v;
    state->last_t = t;
    switch(state->type){
        case OSC_SINE:
        default:
            input->value->v = (sin(state->phase * 2 * M_PI) + 1.0) / 2.0 * input->param_values[2]->v
                              + (1.0 - input->param_values[2]->v) * input->param_values[3]->v ;
    }
}

void inp_lfo_del(input_t * input){
    for(int i = 0; i < input->n_params; i++){
        pval_free(input->param_values[i], input->parameters);
    }
    pval_free(input->value, input);
    free(input->param_values);
    free(input->state);
}

color_t green = {
    .r = 0.1,
    .g = 1.0,
    .b = 0.0,
    .a = 0.0
};

const int n_inputs = N_INPUTS;
input_t inputs[N_INPUTS] = {
    {
        .name = "LFO 1",
        .type = INPUT_LFO,
        .default_val = 0.5,
        .n_params = sizeof(inp_lfo_parameters) / sizeof(parameter_t),
        .parameters = inp_lfo_parameters,
        .color = &green,
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    }
};

void update_inputs(float t) {
    for(int i=0; i < n_inputs; i++)
    {
        inputs[i].update(&inputs[i], t);
    }
}


void input_start(){
    for(int i = 0; i < n_inputs; i++){
        inputs[i].init(&inputs[i]);
    }
}

void input_stop(){
    for(int i = 0; i < n_inputs; i++){
        inputs[i].del(&inputs[i]);
    }
}
