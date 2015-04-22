#include <SDL/SDL.h>

#include "core/err.h"
#include "core/slot.h"
#include "hits/hit.h"

#define N_SLOTS 8
#define N_HIT_SLOTS 8

int n_slots = N_SLOTS;
slot_t slots[N_SLOTS];

int n_hit_slots = N_HIT_SLOTS;
slot_t hit_slots[N_HIT_SLOTS];

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
            c.a *= param_state_get(&slots[i].alpha);
            result.r = result.r * (1 - c.a) + c.r * c.a;
            result.g = result.g * (1 - c.a) + c.g * c.a;
            result.b = result.b * (1 - c.a) + c.b * c.a;
        }
    }

    return render_composite_hits(result, x, y);
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

void update_hits(float t)
{
    if(SDL_LockMutex(hits_updating)) FAIL("Could not lock mutex: %s\n", SDL_GetError());
    for(int i=0; i < N_MAX_ACTIVE_HITS; i++)
    {
        if(active_hits[i].hit)
        {
            if(active_hits[i].hit->update(&active_hits[i], t))
                active_hits[i].hit->stop(&active_hits[i]);
        }
    }

    SDL_UnlockMutex(hits_updating);
}

void pat_load(slot_t* slot, pattern_t* pattern)
{
    slot->pattern = pattern;
    param_state_init(&slot->alpha, 0);
    slot->param_states = malloc(sizeof(param_state_t) * pattern->n_params);
    for(int i = 0; i < pattern->n_params; i++){
        param_state_init(&slot->param_states[i], pattern->parameters[i].default_val);
    }
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

void hit_load(slot_t * slot, hit_t * hit){
    slot->hit = hit;
    param_state_init(&slot->alpha, 0);
    slot->param_states = malloc(sizeof(param_state_t) * hit->n_params);
    for(int i = 0; i < hit->n_params; i++){
        param_state_init(&slot->param_states[i], hit->parameters[i].default_val);
    }
    //slot->state
}

void hit_unload(slot_t * slot){
    if(!slot->hit) return;
    free(slot->param_states);
    //slot->state
    slot->hit = 0;
}
