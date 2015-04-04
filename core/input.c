#include "input.h"

#define N_INPUTS 1

enum osc_type {
    OSC_SINE,
    OSC_SQUARE,
    OSC_TRIANGLE,
    OSC_SAWTOOTH,
};

typedef struct
{
    float phase;
    enum osc_type type;
    float * param_values;
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
};

void inp_lfo_init(input_t * input){
    input->state = malloc(sizeof(inp_lfo_state_t));

    input->param_values = malloc(sizeof(float) * input->n_params);
    for(int i = 0; i < input->n_params; i++){
        input->param_values[i] = input->parameters[i].default_val;
    }
}

void inp_lfo_update(input_t * input, float t){

}

void inp_lfo_del(input_t * input){
    free(input->param_values);
    free(input->state);
}

const int n_inputs = N_INPUTS;
input_t inputs[N_INPUTS] = {
    {
        .name = "LFO 1",
        .type = INPUT_LFO,
        .value = 0.5,
        .n_params = sizeof(inp_lfo_parameters) / sizeof(parameter_t),
        .parameters = inp_lfo_parameters,
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    }
};

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
