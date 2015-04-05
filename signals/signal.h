#ifndef __SIGNAL_H
#define __SIGNAL_H

#include "core/slot.h"

struct signal;

enum signal_type {
    SIGNAL_LFO,
};

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();

typedef void * inp_state_pt;
typedef void (*inp_init_fn_pt)(struct signal * signal);
typedef void (*inp_update_fn_pt)(struct signal * signal, float t);
typedef void (*inp_del_fn_pt)(struct signal * signal);

typedef struct signal {
    char * name;
    enum signal_type type;
    param_state_t * output;
    float default_val;
    int n_params;
    parameter_t * parameters;
    param_state_t ** param_states;
    color_t color;
    inp_state_pt state;
    inp_init_fn_pt init;
    inp_update_fn_pt update;
    inp_del_fn_pt del;
} signal_t;

extern int n_signals;
extern signal_t signals[];

void signal_start();
void signal_stop();
void update_signals(float t);

#endif
