#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <SDL/SDL.h>

#define PVAL_STACK_SIZE 2048
#define PVAL_STACK_DEBUG 

// ---- param_state ----

typedef struct param_state {
    float value;
    SDL_Color handle_color;
    SDL_Color label_color;
    char * label;
} param_state_t;

// ---- parameter ----

typedef void (*param_val_to_str_fn_pt)(float val, char* buf, int n);

typedef struct parameter
{
    char* name;
    param_val_to_str_fn_pt val_to_str;
    float default_val;
} parameter_t;


#endif

