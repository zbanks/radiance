#include <math.h>

#include "core/parameter.h"
#include "util/math.h"

static param_output_t * param_output_list;

void param_output_init(param_output_t * output, float value){
    output->value = value;
    output->connected_state = NULL;
    //output->handle_color = (SDL_Color) {80, 80, 80, 255};
    //output->label_color = (SDL_Color) {80, 80, 80, 255};
    if(!output->label) output->label = "";

    output->next = param_output_list;
    param_output_list = output;
}

void param_output_set(param_output_t * output, float value){
    param_state_t * pstate = output->connected_state;
    // Set local copy
    output->value = value;
    while(pstate){
        // Iterate through linked list setting `value`
        //pstate->value = value;
        switch(pstate->mode){
            case PARAM_VALUE_SCALED:
                pstate->value = value * (pstate->max - pstate->min) + pstate->min;
            break;
            case PARAM_VALUE_EXPANDED:
                pstate->value = (value - pstate->min) / (pstate->max - pstate->min);
            break;
            default:
            case PARAM_VALUE_DIRECT:
                pstate->value = value;
            break;
        }
        pstate->value = MIN(MAX(pstate->value, 0.), 1.0);
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

    // Remove from param_output_list
    param_output_t * poptr = param_output_list;
    while(poptr && poptr->next != output)
        poptr = poptr->next;
    if(poptr)
        poptr->next = output->next;
}

void param_state_init(param_state_t * state, float value){
    state->value = value;
    state->min = 0.;
    state->max = 1.;
    state->connected_output = 0;
    state->next_connected_state = 0;
    state->prev_connected_state = 0;
}

void param_state_connect(param_state_t * state, param_output_t * output){
    // Disconnect if already connected
    param_state_disconnect(state);

    // Set `connected_output` pointer
    state->connected_output = output;

    // Insert `state` into linked list
    if(output->connected_state)
        output->connected_state->prev_connected_state = state;
    state->next_connected_state = output->connected_state;
    state->prev_connected_state = 0;

    output->connected_state = state;
}

int param_state_connect_label(param_state_t * state, const char * label){
    param_output_t * poptr = param_output_list;
    while(poptr && strcmp(poptr->label, label) != 0)
        poptr = poptr->next;
    if(!poptr) return -1;

    param_state_connect(state, poptr);
    return 0;
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

int quantize_parameter(quant_labels_t l, float p){ 
    int i = 0;
    int r;
    while(l[++i]);
    r = floor(p * i);
    if(r >= i) r = i - 1;
    return r;
}

void float_to_string(float val, char * buf, int n){
    snprintf(buf, n, "%f", val);
}

static quant_labels_t power_quant_labels = {
    "0",
    "1 / 16",
    "1 / 8",
    "1 / 4",
    "1 / 2",
    "  1  ",
    "  2 x",
    "  4 x",
    "  8 x",
    LABELS_END
};

void power_quantize_parameter_label(float val, char * buf, int n){
    int v = quantize_parameter(power_quant_labels+1, val);
    strncpy(buf, power_quant_labels[v+1], n);
}

float power_quantize_parameter(float p){
    int v = quantize_parameter(power_quant_labels+1, p);
    return powf(2, v) / 16.0;
}

void power_zero_quantize_parameter_label(float val, char * buf, int n){
    int v = quantize_parameter(power_quant_labels, val);
    strncpy(buf, power_quant_labels[v], n);
}

float power_zero_quantize_parameter(float p){
    int v = quantize_parameter(power_quant_labels, p);
    if(v == 0) return 0;
    return powf(2, v-1) / 16.0;
}
