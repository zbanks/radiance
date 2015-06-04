#ifndef __PATTERN_H
#define __PATTERN_H

#include "core/slot.h"
#include "core/time.h"
#include "util/color.h"

struct pattern;
struct slot;

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();
typedef void (*pat_update_fn_pt)(struct slot* slot, mbeat_t t);
typedef void (*pat_prevclick_fn_pt)(struct slot* slot, float x, float y);
typedef color_t (*pat_render_fn_pt)(struct slot* slot, float x, float y);
typedef void (*pat_del_fn_pt)(pat_state_pt state);

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_prevclick_fn_pt prevclick;
    pat_render_fn_pt render;
    pat_del_fn_pt del;
    int n_params;
    parameter_t* parameters;
    char* name;
} pattern_t;

#ifndef SHAREDLIB
void pattern_init();
void pattern_del();

extern int n_patterns;
extern pattern_t** patterns;

extern pattern_t pat_full;
extern pattern_t pat_wave;
extern pattern_t pat_bubble;
extern pattern_t pat_strobe;
#endif

#endif
