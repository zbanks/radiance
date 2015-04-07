#include <math.h>

#include "core/parameter.h"

void param_output_set(param_output_t * output, float value){
    param_state_t * pstate = output->connected_state;
    // Set local copy
    output->value = value;
    while(pstate){
        // Iterate through linked list setting `value`
        pstate->value = value;
        pstate = pstate->next_connected_state;
    }
}

void param_output_free(param_output_t * output){
    param_state_t * pstate = output->connected_state;
    param_state_t * last_pstate;
    while(pstate){
        // Iterate through linked list and clear `connected_output` & `next_connected state`
        pstate->connected_output = 0;
        last_pstate = pstate;
        pstate = pstate->next_connected_state;
        last_pstate->next_connected_state = 0;
        last_pstate->prev_connected_state = 0;
    }
}

float param_state_get(param_state_t * state){
    return state->value;
}

void param_state_connect(param_state_t * state, param_output_t * output){
    // Set `connected_output` pointer
    state->connected_output = output;

    // Insert `state` into linked list
    if(output->connected_state)
        output->connected_state->prev_connected_state = state;
    state->next_connected_state = output->connected_state;
    state->prev_connected_state = 0;

    output->connected_state = state;
}

void param_state_disconnect(param_state_t * state){
    param_state_t * s;
    param_output_t * output = param_state_output(state);
    if(!output) return;

    // Delete `state` from linked list
    s = state->prev_connected_state;
    if(s)
        s->next_connected_state = state->next_connected_state;
    else
        output->connected_state = state->next_connected_state;

    s = state->next_connected_state;
    if(s)
        s->prev_connected_state = state->prev_connected_state;

    // Clear references to `connected_output` & `next_connected_state`
    state->next_connected_state = 0;
    state->prev_connected_state = 0;
    state->connected_output = 0;

}

param_output_t * param_state_output(param_state_t * state){
    return state->connected_output;
}

void float_to_string(float val, char * buf, int n){
    snprintf(buf, n, "%f", val);
}

void quant_osc_to_string(float val, char * buf, int n){
    snprintf(buf, n, "Sine");
}

int quantize_parameter(quant_labels_t l, float p){ 
    int i = 0;
    int r;
    while(l[++i]);
    r = floor(p * i);
    if(r >= i) r = i - 1;
    return r;
}

