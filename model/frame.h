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

typedef color_t (*pat_render_fn_pt)(float x, float y, pat_state_pt state);

extern const int n_slots;
extern pat_render_fn_pt pat_render_fns[];
extern pat_state_pt pat_states[];

color_t render_composite(float x, float y);

#endif
