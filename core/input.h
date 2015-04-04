#ifndef __INPUT_H
#define __INPUT_H

#include "slot.h"

struct input;

enum input_type {
    INPUT_LFO,
    INPUT_AUDIO,
    INPUT_EXT
};

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();

typedef void * inp_state_pt;
typedef void (*inp_init_fn_pt)(struct input * input);
typedef void (*inp_update_fn_pt)(struct input * input, float t);
typedef void (*inp_del_fn_pt)(struct input * input);

typedef struct input
{
    const char * name;
    enum input_type type;
    float value;
    int n_params;
    parameter_t * parameters;
    float * param_values;
    inp_state_pt state;
    inp_init_fn_pt init;
    inp_update_fn_pt update;
    inp_del_fn_pt del;
} input_t;

extern const int n_inputs;
extern input_t inputs[];

void input_start();
void input_stop();

#endif
