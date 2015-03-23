#include "slot.h"
#include "err.h"

#define N_SLOTS 8

const int n_slots = N_SLOTS;
slot_t slots[N_SLOTS];

pthread_mutex_t patterns_updating;

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
    pthread_mutex_lock(&patterns_updating);

    for(int i=0; i < n_slots; i++)
    {
        if(slots[i].pattern)
        {
            (*slots[i].pattern->update)(&slots[i], t);
        }
    }

    pthread_mutex_unlock(&patterns_updating);    
}

void pat_load(slot_t* slot, pattern_t* pattern)
{
    slot->pattern = pattern;
    slot->state = (*pattern->init)();
    if(!slot->state) FAIL("Could not malloc pattern state\n");
    slot->param_values = malloc(sizeof(float) * pattern->n_params);
    if(!slot->param_values) FAIL("Could not malloc param values\n");
    for(int i=0; i < pattern->n_params; i++)
    {
        slot->param_values[i] = pattern->parameters[i].default_val;
    }
}

void pat_unload(slot_t* slot)
{
    if(!slot->pattern) return;
    (*slot->pattern->del)(slot->state);
    free(slot->param_values);
    slot->pattern = 0;
}

