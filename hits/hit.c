#include <stdlib.h>
#include <math.h>

#include "core/parameter.h"
#include "core/slot.h"
#include "hits/hit.h"
#include "util/color.h"
#include "util/siggen.h"

hit_t * hits[] = {&hit_full, };
int n_hits = sizeof(hits) / sizeof(hit_t);

int n_active_hits = 0;
struct active_hit active_hits[N_MAX_ACTIVE_HITS];

#define N_HIT_SLOTS 8 //FIXME

struct active_hit preview_active_hits[N_HIT_SLOTS];

enum adsr_state {
    ADSR_OFF = 0,
    ADSR_INIT = 0,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
};

color_t render_composite_hits(color_t base, float x, float y) {
    color_t result;

    result.r = 0;
    result.g = 0;
    result.b = 0;

    for(int i=0; i < n_active_hits; i++) {
        color_t c = (active_hits[i].hit->render)(&active_hits[i], x, y);
        c.a *= active_hits[i].alpha;
        result.r = result.r * (1 - c.a) + c.r * c.a;
        result.g = result.g * (1 - c.a) + c.g * c.a;
        result.b = result.b * (1 - c.a) + c.b * c.a;
    }

    return result;
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

int hit_full_start(slot_t * slot, struct active_hit * active_hit) {
    if(!slot->hit) return 1;

    active_hit->param_values = malloc(slot->hit->n_params * sizeof(float));
    active_hit->state = malloc(sizeof(struct hit_full_state));
    active_hit->alpha = slot->alpha;
    
    if(!active_hit->param_values) return 1;
    if(!active_hit->state) return 1;

    for(int i = 0; i < slot->hit->n_params; i++){
        active_hit->param_values[i] = slot->param_states[i].value;
    }

    struct hit_full_state * state = active_hit->state;

    state->color = param_to_color(active_hit->param_values[FULL_COLOR]);
    state->adsr = ADSR_INIT;
    state->x = 0;
    state->base_alpha = state->color.a;

    return 0;
}

void hit_full_stop(struct active_hit * active_hit){
    struct hit_full_state * state = active_hit->state;
    free(active_hit->param_values);
    free(active_hit->state);
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

