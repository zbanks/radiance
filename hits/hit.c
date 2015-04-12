#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "core/slot.h"
#include "hits/hit.h"
#include "util/color.h"
#include "util/siggen.h"
#include "core/err.h"

#define N_HITS  1

hit_t * hits[N_HITS] = {&hit_full, };
int n_hits = N_HITS;

int n_active_hits = 0;
struct active_hit active_hits[N_MAX_ACTIVE_HITS];

#define N_HIT_SLOTS 8 //FIXME

struct active_hit preview_active_hits[N_HIT_SLOTS];

SDL_mutex* hits_updating;

enum adsr_state {
    ADSR_OFF = 0,
    ADSR_INIT = 0,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
};

color_t render_composite_hits(color_t base, float x, float y) {
    color_t result = base;

    for(int i=0; i < n_active_hits; i++) {
        if(!active_hits[i].hit) continue;
        color_t c = (active_hits[i].hit->render)(&active_hits[i], x, y);
        c.a *= active_hits[i].alpha;
        result.r = result.r * (1 - c.a) + c.r * c.a;
        result.g = result.g * (1 - c.a) + c.g * c.a;
        result.b = result.b * (1 - c.a) + c.b * c.a;
    }

    return result;
}

static struct active_hit * alloc_hit(hit_t * hit){
    struct active_hit * ah;

    if(n_active_hits == N_MAX_ACTIVE_HITS){
        ah = &active_hits[N_MAX_ACTIVE_HITS-1];
        ah->hit->stop(ah);

        if(n_active_hits >= N_MAX_ACTIVE_HITS){
            printf("Failed to dealloc hit");
            return 0;
        }
    }

    memcpy(active_hits+1, active_hits, n_active_hits * sizeof(struct active_hit));
    n_active_hits++;

    ah = &active_hits[0];

    ah->hit = hit;
    ah->param_values = malloc(hit->n_params * sizeof(float));
    if(!ah->param_values) return 0;

    return ah;
}

static void free_hit(struct active_hit * active_hit){
    int i;
    if(!active_hit->hit) return;

    active_hit->hit = 0;
    free(active_hit->param_values);
    active_hit->param_values = 0;

    for(i = 0; i < n_active_hits; i++){
        if(active_hit == &active_hits[i]) break;
    }
    n_active_hits--;
    if(i < n_active_hits)
        memcpy(&active_hits[i], &active_hits[i+1], sizeof(struct active_hit) * (n_active_hits -  i));
    memset(&active_hits[n_active_hits], 0, sizeof(struct active_hit));

}

// ----- Hit: Full -----
//
enum hit_full_param_names {
    FULL_COLOR,
    FULL_ATTACK,
    FULL_DECAY,
    FULL_SUSTAIN,
    FULL_RELEASE,

    N_FULL_PARAMS
};

parameter_t hit_full_params[] = {
    [FULL_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [FULL_ATTACK] = {
        .name = "Attack",
        .default_val = 0.3,
    },
    [FULL_DECAY] = {
        .name = "Decay",
        .default_val = 0.1,
    },
    [FULL_SUSTAIN] = {
        .name = "Sustain",
        .default_val = 0.7,
    },
    [FULL_RELEASE] = {
        .name = "Release",
        .default_val = 0.5,
    },
};

struct hit_full_state {
    color_t color;
    enum adsr_state adsr;
    float x;
    float last_t;
    float base_alpha;
};

struct active_hit * hit_full_start(slot_t * slot) {
    if(!slot->hit) return 0;

    struct active_hit * active_hit = alloc_hit(slot->hit);
    if(!active_hit) return 0;

    active_hit->state = malloc(sizeof(struct hit_full_state));
    if(!active_hit->state) return 0;

    active_hit->alpha = slot->alpha;

    for(int i = 0; i < slot->hit->n_params; i++){
        active_hit->param_values[i] = slot->param_states[i].value;
    }

    struct hit_full_state * state = active_hit->state;

    state->color = param_to_color(active_hit->param_values[FULL_COLOR]);
    state->adsr = ADSR_INIT;
    state->x = 0;
    state->base_alpha = state->color.a;

    return active_hit;
}

void hit_full_stop(struct active_hit * active_hit){
    if(active_hit->state) free(active_hit->state);
    active_hit->state = 0;
    free_hit(active_hit);
}

int hit_full_update(struct active_hit * active_hit, float t){
    struct hit_full_state * state = active_hit->state;
    switch(state->adsr){
        case ADSR_INIT:
            state->adsr = ADSR_ATTACK;
        break;
        case ADSR_ATTACK:
            state->x += (t - state->last_t) * (state->base_alpha) / active_hit->param_values[FULL_ATTACK];
            if(state->x >= state->base_alpha){
                state->x = state->base_alpha;
                state->adsr = ADSR_DECAY;
            }
        break;
        case ADSR_DECAY:
            state->x -= (t - state->last_t) * (state->base_alpha * active_hit->param_values[FULL_SUSTAIN]) / active_hit->param_values[FULL_DECAY];
            if(state->x <= (state->base_alpha * active_hit->param_values[FULL_SUSTAIN])){
                state->x = state->base_alpha * active_hit->param_values[FULL_SUSTAIN];
                state->adsr = ADSR_SUSTAIN;
            }
        break;
        case ADSR_SUSTAIN:
        break;
        case ADSR_RELEASE:
            state->x -= (t - state->last_t) * (state->base_alpha * active_hit->param_values[FULL_SUSTAIN]) / active_hit->param_values[FULL_RELEASE];
            if(state->x <= 0){
                state->x = 0.;
                return 1;
            }
        break;
    }
    state->last_t = t;
    return 0;
}

int hit_full_event(struct active_hit * active_hit, enum hit_event event, float event_data){
    struct hit_full_state * state = active_hit->state;
    switch(event){
        case HITEV_NOTE_ON:
            state->adsr = ADSR_ATTACK;
        break;
        case HITEV_NOTE_OFF:
            state->adsr = ADSR_RELEASE;
        break;
        default: break;
    }
    return 0;
}

void hit_full_prevclick(slot_t * slot, float x, float y){

}

color_t hit_full_pixel(struct active_hit * active_hit, float x, float y) {
    struct hit_full_state * state = active_hit->state;
    color_t color = state->color;
    color.a = state->base_alpha * state->x;
    return color;
}

hit_t hit_full = {
    .render = &hit_full_pixel,
    .start = &hit_full_start,
    .stop = &hit_full_stop,
    .update = &hit_full_update,
    .event = &hit_full_event,
    .prevclick = &hit_full_prevclick,
    .n_params = N_FULL_PARAMS,
    .parameters = hit_full_params,
    .name = "Full Hit",
};

