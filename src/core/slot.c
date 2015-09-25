#include <SDL/SDL.h>
#include <math.h>

#include "core/err.h"
#include "core/slot.h"
#include "core/time.h"
#include "core/config.h"

#define N_SLOTS 8

int n_slots = N_SLOTS;
slot_t slots[N_SLOTS]; // Initialize everything to zero

SDL_mutex* patterns_updating;

void render_composite_frame(state_source_t src, float * x, float * y, size_t n, color_t * out){
    memset(out, 0, n * sizeof(color_t)); // Initialize to black
    int has_solo = 0;
    for(int i = 0; i < n_slots; i++){
        has_solo |= slots[i].solo;
    }

    for(int i = 0; i < n_slots; i++){
        if(!slots[i].pattern) continue;
        if(slots[i].mute) continue;
        if(has_solo && !slots[i].solo) continue;
        const pat_state_pt pat_state_p = (src == STATE_SOURCE_UI) ? slots[i].ui_state : slots[i].state;
        const pat_render_fn_pt pat_render = *slots[i].pattern->render;
        float slot_alpha = param_state_get(&slots[i].alpha);
        slot_alpha = pow(slot_alpha, config.render.alpha_gamma);

        if(slot_alpha > 1e-4){
            for(size_t j = 0; j < n; j++){
                color_t c = pat_render(pat_state_p, x[j], y[j]);
                c.a *= slot_alpha;
                out[j].r = out[j].r * (1. - c.a) + c.r * c.a;
                out[j].g = out[j].g * (1. - c.a) + c.g * c.a;
                out[j].b = out[j].b * (1. - c.a) + c.b * c.a;
            }
        }
    }
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

void update_ui()
{
    if(SDL_LockMutex(patterns_updating)) FAIL("Could not lock mutex: %s\n", SDL_GetError());

    for(int i=0; i < n_slots; i++)
    {
        if(slots[i].pattern)
        {
            memcpy(slots[i].ui_state, slots[i].state, slots[i].pattern->state_size);
        }
    }

    SDL_UnlockMutex(patterns_updating); 
}

void pat_load(slot_t* slot, pattern_t* pattern)
{
    if(slot->pattern) pat_unload(slot);

    slot->pattern = pattern;

    slot->state = malloc(pattern->state_size);
    if(!slot->state) FAIL("Could not malloc pattern state\n");
    (*pattern->init)(slot->state);

    if(config.ui.enabled)
    {
        slot->ui_state = malloc(pattern->state_size);
        if(!slot->ui_state) FAIL("Could not malloc pattern state\n");
    }

    slot->colormap = NULL;

    if(pattern->n_params > N_MAX_PARAMS) FAIL("Pattern '%s' requires %d parameters; max is %d.\n", pattern->name, pattern->n_params, N_MAX_PARAMS);

    param_state_setq(&slot->alpha, 0.);
    for(int i = 0; i < pattern->n_params; i++){
        param_state_setq(&slot->param_states[i], pattern->parameters[i].default_val);
    }
}

void pat_unload(slot_t* slot)
{
    if(!slot->pattern) return;
    slot->colormap = NULL;

    /*
    for(int i = 0; i < slot->pattern->n_params; i++){
        param_state_disconnect(&slot->param_states[i]);
    }
    param_state_disconnect(&slot->alpha);
    */

    free(slot->state);
    if(config.ui.enabled)
    {
        free(slot->ui_state);
    }
    slot->pattern = 0;
}

void slots_init(){
    for(int i = 0; i < n_slots; i++){
        slots[i].pattern = NULL;
        slots[i].state = NULL;
        slots[i].colormap = NULL;
        param_state_init(&slots[i].alpha, 0.);
        for(int j = 0; j < N_MAX_PARAMS; j++){
            param_state_init(&slots[i].param_states[j], 0.);
        }
    }
}

void slots_del(){
    for(int i = 0; i < n_slots; i++){
        pat_unload(&slots[i]);
        param_state_disconnect(&slots[i].alpha);
        for(int j = 0; j < N_MAX_PARAMS; j++){
            param_state_disconnect(&slots[i].param_states[j]);
        }
    }
}
