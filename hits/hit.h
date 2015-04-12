#ifndef __HIT_H_
#define __HIT_H_

#include "core/slot.h"
#include "core/parameter.h"

#define N_MAX_ACTIVE_HITS 128

struct slot;
struct hit;
struct active_hit;

enum hit_event {
    HITEV_NONE = 0,
    HITEV_NOTE_ON,
    HITEV_NOTE_OFF,
    HITEV_AFTERTOUCH
};

typedef void * hit_state_pt;
typedef void * active_hit_state_pt;
//typedef hit_state_pt (*hit_init_fn_pt)();
typedef int (*hit_start_fn_pt)(struct slot * slot, struct active_hit * active_hit);
typedef int (*hit_update_fn_pt)(struct active_hit * active_hit, float t);
typedef int (*hit_event_fn_pt)(struct active_hit * active_hit, enum hit_event event, float event_data);
typedef void (*hit_prevclick_fn_pt)(struct slot * slot, float x, float y);
typedef color_t (*hit_render_fn_pt)(struct active_hit * active_hit, float x, float y);
typedef void (*hit_stop_fn_pt)(struct active_hit * active_hit);
//typedef void (*hit_del_fn_pt)(hit_state_pt state);

typedef struct hit {
    //hit_init_fn_pt init;
    hit_start_fn_pt start;
    hit_update_fn_pt update;
    hit_event_fn_pt event;
    hit_prevclick_fn_pt prevclick;
    hit_render_fn_pt render;
    hit_stop_fn_pt stop;
    //hit_del_fn_pt del;
    int n_params;
    parameter_t * parameters;
    char * name;
} hit_t;

struct active_hit {
    struct hit * hit;
    float alpha;
    float * param_values;
    void * state;
};

color_t render_composite_hits(color_t base, float x, float y);

extern int n_hits;
extern struct hit * hits[];

extern int n_active_hits;
extern struct active_hit active_hits[];

extern struct active_hit preview_active_hits[];

extern hit_t hit_full;

#endif
