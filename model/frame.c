#include "frame.h"

#define N_SLOTS 8
const int n_slots = N_SLOTS;
pat_render_fn_pt pat_render_fns[N_SLOTS];
pat_state_pt pat_states[N_SLOTS];

color_t render_composite(float x, float y)
{
    color_t result;

    result.r = 0;
    result.g = 0;
    result.b = 0;

    for(int i=0; i < n_slots; i++)
    {
        pat_render_fn_pt render = pat_render_fns[i];
        if(render)
        {
            color_t c = (*render)(x, y, pat_states[i]);
            result.r = result.r * (1 - c.a) + c.r * c.a;
            result.g = result.g * (1 - c.a) + c.g * c.a;
            result.b = result.b * (1 - c.a) + c.b * c.a;
        }
    }
    return result;
}

