#include "frame.h"

#define N_SLOTS 8
const int n_slots = N_SLOTS;
slot_t slots[N_SLOTS];

color_t render_composite(float x, float y)
{
    color_t result;

    result.r = 0;
    result.g = 0;
    result.b = 0;

    for(int i=0; i < n_slots; i++)
    {
        if(slots[i].pattern)
        {
            color_t c = (*slots[i].pattern->render)(&slots[i], x, y);
            result.r = result.r * (1 - c.a) + c.r * c.a;
            result.g = result.g * (1 - c.a) + c.g * c.a;
            result.b = result.b * (1 - c.a) + c.b * c.a;
        }
    }
    return result;
}

void update_patterns(float t)
{
    for(int i=0; i < n_slots; i++)
    {
        if(slots[i].pattern)
        {
            (*slots[i].pattern->update)(&slots[i], t);
        }
    }
}

