#include "slot.h"
#include "err.h"
#include <SDL/SDL.h>

#define N_SLOTS 8

const int n_slots = N_SLOTS;
slot_t slots[N_SLOTS];

// Parameter value pointer allocation
pval_t pvals[PVAL_STACK_SIZE];

static pval_t * next_pval = 0;
static int free_pvals = PVAL_STACK_SIZE;

void pval_init_stack(){
    for(int i = 0; i < PVAL_STACK_SIZE; i++){
        pvals[i].next = next_pval;
        next_pval = &pvals[i];
    }
}

pval_t * pval_new(float v, void * owner){
    struct pval * pv;
    if(!free_pvals)
        return 0;
    if(next_pval == 0)
        pval_init_stack();
    pv = next_pval;
    next_pval = next_pval->next;
    pv->v = v;
    pv->owner = owner;
    free_pvals--;
    printf("free pvals-- %d\n", free_pvals);
    return pv;
}

void pval_free(pval_t * pv, void * owner){
    if(pv->owner != owner)
        return;
    pv->next = next_pval;
    next_pval = pv;
    free_pvals++;
    printf("free pvals++: %d\n", free_pvals);
}

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
    slot->param_values = malloc(sizeof(float *) * pattern->n_params);
    if(!slot->param_values) FAIL("Could not malloc param values\n");
    for(int i=0; i < pattern->n_params; i++)
    {
        slot->param_values[i] = pval_new(pattern->parameters[i].default_val, slot);
    }
    slot->state = (*pattern->init)();
    if(!slot->state) FAIL("Could not malloc pattern state\n");
}

void pat_unload(slot_t* slot)
{
    if(!slot->pattern) return;
    for(int i=0; i < slot->pattern->n_params; i++){
        pval_free(slot->param_values[i], slot);
    }
    (*slot->pattern->del)(slot->state);
    free(slot->param_values);
    slot->pattern = 0;
}

