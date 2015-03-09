#ifndef __FRAME_H
#define __FRAME_H

typedef struct color
{
    float r;
    float g;
    float b;
    float a;
} color_t;

typedef void* pat_state_pt;

typedef void (*pat_init_fn_pt)(pat_state_pt state);
typedef void (*pat_update_fn_pt)(float t, pat_state_pt state);
typedef color_t (*pat_render_fn_pt)(float x, float y, pat_state_pt state);
typedef void (*pat_del_fn_pt)(pat_state_pt state);

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_render_fn_pt render;
    pat_del_fn_pt del;
} pattern_t;

typedef struct slot
{
    const pattern_t* pattern;
    void* state;
} slot_t;

extern const int n_slots;

extern slot_t slots[];

color_t render_composite(float x, float y);

#endif
