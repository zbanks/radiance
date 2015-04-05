#include <SDL/SDL.h>

#include "core/err.h"
#include "core/slot.h"

#define N_SLOTS 8

int n_slots = N_SLOTS;
slot_t slots[N_SLOTS];

SDL_mutex* patterns_updating;

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
            c.a *= slots[i].alpha;
            result.r = result.r * (1 - c.a) + c.r * c.a;
            result.g = result.g * (1 - c.a) + c.g * c.a;
            result.b = result.b * (1 - c.a) + c.b * c.a;
        }
    }
    return result;
}

void update_patterns(float t)
{
    if(SDL_LockMutex(patterns_updating)) FAIL("Could not lock mutex: %s\n", SDL_GetError());

    for(int i=0; i < n_slots; i++)
    {
        if(slots[i].pattern)
        {
            (*slots[i].pattern->update)(&slots[i], t);
        }
    }

    SDL_UnlockMutex(patterns_updating); 
}

void pat_load(slot_t* slot, pattern_t* pattern)
{
    slot->pattern = pattern;
    slot->alpha = 0;
    slot->param_states = malloc(sizeof(param_state_t *) * pattern->n_params);
    slot->state = (*pattern->init)();
    if(!slot->state) FAIL("Could not malloc pattern state\n");
}

void pat_unload(slot_t* slot)
{
    if(!slot->pattern) return;
    (*slot->pattern->del)(slot->state);
    free(slot->param_states);
    slot->pattern = 0;
}

