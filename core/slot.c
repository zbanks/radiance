#include <SDL/SDL.h>

#include "core/err.h"
#include "core/slot.h"
#include "core/time.h"

#define N_SLOTS 8

int n_slots = N_SLOTS;
slot_t slots[N_SLOTS]; // Initialize everything to zero

SDL_mutex* patterns_updating;

void render_composite_frame(slot_t * fslots, float * x, float * y, size_t n, color_t * out){
    memset(out, 0, n * sizeof(color_t)); // Initialize to black
    for(int i = 0; i < n_slots; i++){
        if(!fslots[i].pattern) continue;

        for(size_t j = 0; j < n; j++){
            color_t c = (*fslots[i].pattern->render)(&fslots[i], x[j], y[j]);
            c.a *= param_state_get(&fslots[i].alpha);
            out[j].r = out[j].r * (1. - c.a) + c.r * c.a;
            out[j].g = out[j].g * (1. - c.a) + c.g * c.a;
            out[j].b = out[j].b * (1. - c.a) + c.b * c.a;
        }
    }
}

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

    return result;
}

void update_patterns(mbeat_t t)
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
    if(slot->pattern) pat_unload(slot);

    slot->pattern = pattern;

    slot->state = (*pattern->init)();
    if(!slot->state) FAIL("Could not malloc pattern state\n");

    slot->colormap = NULL;
    //param_state_init(&slot->alpha, 0.);

    slot->param_states = malloc(sizeof(param_state_t) * pattern->n_params);
    for(int i = 0; i < pattern->n_params; i++){
        param_state_init(&slot->param_states[i], pattern->parameters[i].default_val);
    }
}

void pat_unload(slot_t* slot)
{
    if(!slot->pattern) return;
    slot->colormap = NULL;

    for(int i = 0; i < slot->pattern->n_params; i++){
        param_state_disconnect(&slot->param_states[i]);
    }
    free(slot->param_states);

    //param_state_disconnect(&slot->alpha);

    (*slot->pattern->del)(slot->state);
    slot->pattern = 0;
}

