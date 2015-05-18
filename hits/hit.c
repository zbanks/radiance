#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL_mutex.h>

#include "core/parameter.h"
#include "core/slot.h"
#include "hits/hit.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"
#include "core/err.h"

#ifndef M_PI 
#define M_PI 3.1415
#endif


#define N_HITS  3

hit_t * hits[N_HITS] = {&hit_full, &hit_pulse, &hit_circle};
int n_hits = N_HITS;

int n_active_hits = 0;
struct active_hit active_hits[N_MAX_ACTIVE_HITS];



#define N_HIT_SLOTS 8 //FIXME

struct active_hit preview_active_hits[N_HIT_SLOTS];

SDL_mutex* hits_updating;

enum adsr_state {
    ADSR_OFF = 0,
    ADSR_WAITING = 0,
    ADSR_START,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
    ADSR_DONE,
};

color_t render_composite_hits(color_t base, float x, float y) {
    color_t result = base;

    for(int i=0; i < N_MAX_ACTIVE_HITS; i++) {
        if(!active_hits[i].hit || !active_hits[i].state) continue;
        color_t c = (active_hits[i].hit->render)(&active_hits[i], x, y);
        c.a *= active_hits[i].alpha;
        result.r = result.r * (1 - c.a) + c.r * c.a;
        result.g = result.g * (1 - c.a) + c.g * c.a;
        result.b = result.b * (1 - c.a) + c.b * c.a;
    }

    return result;
}

color_t render_composite_slot_hits(slot_t * slot, float x, float y) {
    color_t result = {0, 0, 0, 1.0};

    for(int i=0; i < N_MAX_ACTIVE_HITS; i++) {
        if(!active_hits[i].hit || !active_hits[i].state) continue;
        if(active_hits[i].slot != slot) continue;
        color_t c = (active_hits[i].hit->render)(&active_hits[i], x, y);
        //c.a *= active_hits[i].alpha;
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
    }else{
        for(int i = 0; i < N_MAX_ACTIVE_HITS; i++){
            if(!active_hits[i].hit){
                ah = &active_hits[i];
                break;
            }
        }
    }

    ah->param_values = malloc(hit->n_params * sizeof(float));
    if(!ah->param_values) return 0;
    memset(ah->param_values, 0, hit->n_params * sizeof(float));

    ah->hit = hit;
    n_active_hits++;

    return ah;
}

static void free_hit(struct active_hit * active_hit){
    int i;
    if(!active_hit || !active_hit->hit) return;

    free(active_hit->param_values);
    active_hit->param_values = 0;
    memset(active_hit, 0, sizeof(struct active_hit));

    active_hit->hit = 0;
    n_active_hits--;
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
        .name = "Attack Time",
        .default_val = 0.3,
    },
    [FULL_DECAY] = {
        .name = "Decay Time",
        .default_val = 0.1,
    },
    [FULL_SUSTAIN] = {
        .name = "Sustain Level",
        .default_val = 0.7,
    },
    [FULL_RELEASE] = {
        .name = "Release Time",
        .default_val = 0.5,
    },
};

struct hit_full_state {
    color_t color;
    enum adsr_state adsr;
    float x;
    mbeat_t last_t;
    float base_alpha;
};

struct active_hit * hit_full_start(slot_t * slot) {
    if(!slot->hit) return 0;

    struct active_hit * active_hit = alloc_hit(slot->hit);
    if(!active_hit) return 0;

    active_hit->state = malloc(sizeof(struct hit_full_state));
    if(!active_hit->state) return 0;
    struct hit_full_state * state = active_hit->state;

    for(int i = 0; i < slot->hit->n_params; i++){
        active_hit->param_values[i] = slot->param_states[i].value;
    }

    active_hit->param_values[FULL_SUSTAIN] = (active_hit->param_values[FULL_SUSTAIN] * 1.0) + 0.00;
    active_hit->param_values[FULL_ATTACK] = (active_hit->param_values[FULL_ATTACK] / 2.5) + 0.05;
    active_hit->param_values[FULL_DECAY] = (active_hit->param_values[FULL_DECAY] / 5.0) + 0.05;
    active_hit->param_values[FULL_RELEASE] = (active_hit->param_values[FULL_RELEASE] / 1.0) + 0.05;

    state->color = param_to_color(active_hit->param_values[FULL_COLOR]);
    state->adsr = ADSR_WAITING;
    state->x = 0.0;
    state->last_t = 0;
    state->base_alpha = state->color.a;

    active_hit->slot = slot;
    active_hit->alpha = param_state_get(&slot->alpha);


    return active_hit;
}

void hit_full_stop(struct active_hit * active_hit){
    if(active_hit->state) free(active_hit->state);
    active_hit->state = 0;
    free_hit(active_hit);
}

int hit_full_update(struct active_hit * active_hit, mbeat_t t){
    struct hit_full_state * state = active_hit->state;
    switch(state->adsr){
        case ADSR_WAITING: break;
        case ADSR_START:
            state->adsr = ADSR_ATTACK;
        break;
        case ADSR_ATTACK:
            state->x += MB2B(t - state->last_t) / active_hit->param_values[FULL_ATTACK];
            if(state->x >= 1.0){
                state->x = 1.0;
                state->adsr = ADSR_DECAY;
            }
        break;
        case ADSR_DECAY:
            state->x -= MB2B(t - state->last_t) * (1. - active_hit->param_values[FULL_SUSTAIN]) / active_hit->param_values[FULL_DECAY];
            if(state->x <= (active_hit->param_values[FULL_SUSTAIN])){
                state->x = active_hit->param_values[FULL_SUSTAIN];
                state->adsr = ADSR_SUSTAIN;
            }
        break;
        case ADSR_SUSTAIN:
        break;
        case ADSR_RELEASE:
            state->x -= MB2B(t - state->last_t) * (1. - active_hit->param_values[FULL_SUSTAIN]) / active_hit->param_values[FULL_RELEASE];
            if(state->x <= 0.){
                state->x = 0.;
                state->adsr = ADSR_DONE;
                return 1;
            }
        break;
        case ADSR_DONE: break;
    }
    state->last_t = t;
    return 0;
}

int hit_full_event(struct active_hit * active_hit, enum hit_event event, float event_data){
    struct hit_full_state * state = active_hit->state;
    switch(event){
        case HITEV_NOTE_ON:
            state->adsr = ADSR_START;
            state->base_alpha = event_data;
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
    .name = "Full",
};

// ----- Hit: Pulse -----
//
enum hit_pulse_param_names {
    PULSE_COLOR,
    PULSE_TIME,
    PULSE_WIDTH,
    PULSE_ANGLE,

    N_PULSE_PARAMS
};

parameter_t hit_pulse_params[] = {
    [PULSE_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [PULSE_TIME] = {
        .name = "time",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [PULSE_WIDTH] = {
        .name = "Width",
        .default_val = 0.3,
    },
    [PULSE_ANGLE] = {
        .name = "Angle",
        .default_val = 0.5,
    },
};

struct hit_pulse_state {
    color_t color;
    float base_alpha;
    float start_t;
    float x;
};

struct active_hit * hit_pulse_start(slot_t * slot) {
    if(!slot->hit) return 0;

    struct active_hit * active_hit = alloc_hit(slot->hit);
    if(!active_hit) return 0;

    active_hit->state = malloc(sizeof(struct hit_pulse_state));
    if(!active_hit->state) return 0;
    struct hit_pulse_state * state = active_hit->state;

    for(int i = 0; i < slot->hit->n_params; i++){
        active_hit->param_values[i] = slot->param_states[i].value;
    }

    active_hit->param_values[PULSE_TIME] = 1.0 / power_quantize_parameter(active_hit->param_values[PULSE_TIME]);
    active_hit->param_values[PULSE_WIDTH] = (active_hit->param_values[PULSE_WIDTH] / 2.5) + 0.05;
    active_hit->param_values[PULSE_ANGLE] = (active_hit->param_values[PULSE_ANGLE] * 2 * M_PI) - M_PI;

    state->color = param_to_color(active_hit->param_values[PULSE_COLOR]);
    state->start_t = -1;
    state->x = -1.;
    state->base_alpha = state->color.a;

    active_hit->slot = slot;
    active_hit->alpha = param_state_get(&slot->alpha);

    return active_hit;
}

void hit_pulse_stop(struct active_hit * active_hit){
    if(active_hit->state) free(active_hit->state);
    active_hit->state = 0;
    free_hit(active_hit);
}

int hit_pulse_update(struct active_hit * active_hit, mbeat_t abs_t){
    struct hit_pulse_state * state = active_hit->state;
    if(state->start_t < 0)
        state->start_t = abs_t;
    state->x = MB2B(abs_t - state->start_t) / (active_hit->param_values[PULSE_TIME] / 2.) - 1.;
    if(state->x > 1.5){ // Wait until at least sqrt(2) so it fades out nice on the corner
        return 1;
    }
    return 0;
}

int hit_pulse_event(struct active_hit * active_hit, enum hit_event event, float event_data){
    struct hit_pulse_state * state = active_hit->state;
    switch(event){
        case HITEV_NOTE_ON:
            state->base_alpha = event_data;
        break;
        case HITEV_NOTE_OFF:
        break;
        default: break;
    }
    return 0;
}

void hit_pulse_prevclick(slot_t * slot, float x, float y){

}

color_t hit_pulse_pixel(struct active_hit * active_hit, float x, float y) {
    struct hit_pulse_state * state = active_hit->state;
    color_t color = state->color;
    float a;
    float z;
    if((x == 0.) && (y == 0.)){
        a = 0;
    }else{
        //a = (SIN(active_hit->param_values[PULSE_ANGLE]) * y + COS(active_hit->param_values[PULSE_ANGLE]) * x) / (x * x + y * y);
        a = (SIN(active_hit->param_values[PULSE_ANGLE]) * y + COS(active_hit->param_values[PULSE_ANGLE]) * x); // / (x * x + y * y);
    }
    z = a - state->x;
    color.a = state->base_alpha * (fabs(z) < active_hit->param_values[PULSE_WIDTH] ? COS(z * M_PI / (2 * active_hit->param_values[PULSE_WIDTH])) : 0.);
    return color;
}


hit_t hit_pulse = {
    .render = &hit_pulse_pixel,
    .start = &hit_pulse_start,
    .stop = &hit_pulse_stop,
    .update = &hit_pulse_update,
    .event = &hit_pulse_event,
    .prevclick = &hit_pulse_prevclick,
    .n_params = N_PULSE_PARAMS,
    .parameters = hit_pulse_params,
    .name = "Pulse",
};

// ----- Hit: Circle -----
//
enum hit_circle_param_names {
    CIRCLE_COLOR,
    CIRCLE_TIME,
    CIRCLE_WIDTH,

    N_CIRCLE_PARAMS
};

parameter_t hit_circle_params[] = {
    [CIRCLE_COLOR] = {
        .name = "Color",
        .default_val = 0.5,
    },
    [CIRCLE_TIME] = {
        .name = "time",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [CIRCLE_WIDTH] = {
        .name = "Width",
        .default_val = 0.3,
    },
};

struct hit_circle_state {
    color_t color;
    float base_alpha;
    float start_t;
    float x;
};

struct active_hit * hit_circle_start(slot_t * slot) {
    if(!slot->hit) return 0;

    struct active_hit * active_hit = alloc_hit(slot->hit);
    if(!active_hit) return 0;

    active_hit->state = malloc(sizeof(struct hit_circle_state));
    if(!active_hit->state) return 0;
    struct hit_circle_state * state = active_hit->state;

    for(int i = 0; i < slot->hit->n_params; i++){
        active_hit->param_values[i] = slot->param_states[i].value;
    }

    active_hit->param_values[CIRCLE_TIME] = 1.0 / power_quantize_parameter(active_hit->param_values[CIRCLE_TIME]);
    active_hit->param_values[CIRCLE_WIDTH] = (active_hit->param_values[CIRCLE_WIDTH] / 2.5) + 0.05;

    state->color = param_to_color(active_hit->param_values[CIRCLE_COLOR]);
    state->start_t = -1;
    state->x = -1.4;
    state->base_alpha = state->color.a;

    active_hit->slot = slot;
    active_hit->alpha = param_state_get(&slot->alpha);

    return active_hit;
}

void hit_circle_stop(struct active_hit * active_hit){
    if(active_hit->state) free(active_hit->state);
    active_hit->state = 0;
    free_hit(active_hit);
}

int hit_circle_update(struct active_hit * active_hit, mbeat_t abs_t){
    struct hit_circle_state * state = active_hit->state;
    if(state->start_t < 0)
        state->start_t = abs_t;
    state->x = MB2B(abs_t - state->start_t) / (active_hit->param_values[CIRCLE_TIME] / 2.) - 1.;
    if(state->x > 0.1){ // Wait until at least sqrt(2) so it fades out nice on the corner
        return 1;
    }
    return 0;
}

int hit_circle_event(struct active_hit * active_hit, enum hit_event event, float event_data){
    struct hit_circle_state * state = active_hit->state;
    switch(event){
        case HITEV_NOTE_ON:
            state->base_alpha = event_data;
        break;
        case HITEV_NOTE_OFF:
        break;
        default: break;
    }
    return 0;
}

void hit_circle_prevclick(slot_t * slot, float x, float y){

}

color_t hit_circle_pixel(struct active_hit * active_hit, float x, float y) {
    struct hit_circle_state * state = active_hit->state;
    color_t color = state->color;
    float a;
    float z;
    a = sqrt(x * x + y * y);
    z = a + state->x;
    color.a = state->base_alpha * (fabs(z) < active_hit->param_values[CIRCLE_WIDTH] ? COS(z * M_PI / (2 * active_hit->param_values[CIRCLE_WIDTH])) : 0.);
    return color;
}


hit_t hit_circle = {
    .render = &hit_circle_pixel,
    .start = &hit_circle_start,
    .stop = &hit_circle_stop,
    .update = &hit_circle_update,
    .event = &hit_circle_event,
    .prevclick = &hit_circle_prevclick,
    .n_params = N_CIRCLE_PARAMS,
    .parameters = hit_circle_params,
    .name = "Circle",
};
