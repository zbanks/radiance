#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <SDL/SDL.h>

// ---- param_state ----

// Forward declarations
struct param_state;
struct param_output;

typedef struct param_state {
    float value;
    struct param_output * connected_output;
    struct param_state * next_connected_state;
    struct param_state * prev_connected_state;
} param_state_t;

typedef struct param_output {
    float value;
    struct param_state * connected_state;
    SDL_Color handle_color;
    SDL_Color label_color;
    char * label;
} param_output_t;

void param_output_set(param_output_t * output, float value);
void param_output_free(param_output_t * output);
void param_state_init(param_state_t * state, float value);
void param_state_setq(param_state_t * state, float value);
float param_state_get(param_state_t * state);
void param_state_connect(param_state_t * state, param_output_t * output);
void param_state_disconnect(param_state_t * state);
param_output_t * param_state_output(param_state_t * state);  // Returns NULL if not connected

// ---- parameter ----

typedef void (*param_val_to_str_fn_pt)(float val, char* buf, int n);

typedef struct parameter
{
    char* name;
    param_val_to_str_fn_pt val_to_str;
    float default_val;
} parameter_t;

void float_to_string(float val, char * buf, int n);
void quant_osc_to_string(float val, char * buf, int n);

// ---- quantized parameters ----

typedef char *quant_labels_t[];
#define LABELS_END 0

int quantize_parameter(quant_labels_t l, float p);

void power_quantize_parameter_label(float val, char * buf, int n);
float power_quantize_parameter(float p);

#endif

