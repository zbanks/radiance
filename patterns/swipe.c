#include <math.h>
#include <stdlib.h>

#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "util/color.h"
#include "util/math.h"
#include "util/siggen.h"
#include "util/signal.h"

// --------- Pattern: Swipe -----------

enum swipe_state {
    SWIPE_OFF = 0,
    SWIPE_GROWING,
    SWIPE_SHRINKING,
};

enum swipe_source {
    SOURCE_MOUSE,
    SOURCE_M1,
    SOURCE_M2,

    N_SWIPE_SOURCES
};

struct swipe;
struct swipe {
    double kx; // Wave vector, normalized s.t. sqrt(kx * kx + ky * ky) == 1.0
    double ky;
    double ox; // Origin
    double oy; 
    double length; // Length so far, it grows as the button is held
    double alpha;  // Alpha
    enum swipe_state state; // State: growing, shrinking, off
    struct swipe * next;
};

#define N_SWIPE_BUFFER 128

typedef struct {
    //struct freq_state freq_state;
    double freq;
    mbeat_t last_t;
    color_t color;
    double sharp;
    struct swipe * active_swipes[N_SWIPE_SOURCES];
    struct swipe * inactive_swipes;
    struct swipe swipe_buffer[N_SWIPE_BUFFER];
} state_t;

enum param_names {
    COLOR,
    SPEED,
    SHARP,

    N_PARAMS
};

static parameter_t params[] = {
    [COLOR] = {
        .name = "Color",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [SPEED] = {
        .name = "Speed",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
    [SHARP] = {
        .name = "Sharpness",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

static pat_state_pt init() {
    state_t * state = malloc(sizeof(state_t));
    if(!state) FAIL("Unable to malloc state for swipe.\n");
    memset(state, 0, sizeof(state_t));

    struct swipe * sptr = state->inactive_swipes = state->swipe_buffer;
    memset(sptr, 0, sizeof(struct swipe) * N_SWIPE_BUFFER);

    for(int i = 0; i < N_SWIPE_BUFFER-1; i++){
        sptr[i].next = &sptr[i+1];
    }

    return state;
}

static void del(pat_state_pt state) {
    free(state);
}

static void update(slot_t* slot, mbeat_t t) {
    state_t * state = (state_t *) slot->state;
    if(state->last_t == 0) state->last_t = t;

    state->color = colormap_color(cm_global, param_state_get(&slot->param_states[COLOR]));
    //freq_update(&state->freq_state, t, param_state_get(&slot->param_states[SPEED]));
    state->freq = power_quantize_parameter(param_state_get(&slot->param_states[SPEED]));
    state->sharp = param_state_get(&slot->param_states[SHARP]);

    double delta = state->freq * 2 * MB2B(t - state->last_t);

    for(int i = 0; i < N_SWIPE_SOURCES; i++){
        struct swipe ** prev = &state->active_swipes[i];
        struct swipe * swipe = state->active_swipes[i];
        while(swipe){
            switch(swipe->state){
                case SWIPE_GROWING:
                    swipe->length += delta;
                    prev = &swipe->next;
                break;
                case SWIPE_SHRINKING:
                    swipe->ox += delta * swipe->kx;
                    swipe->oy += delta * swipe->ky;
                    if(! ((swipe->ox > -2 && swipe->ox < 2) && (swipe->oy > -2 && swipe->oy < 2))){
                        // Remove swipe, clear it, and put it back in the inactive pile
                        *prev = swipe->next;
                        memset(swipe, 0, sizeof(struct swipe));
                        swipe->next = state->inactive_swipes;
                        state->inactive_swipes = swipe;
                        swipe = *prev;
                    }else{
                        prev = &swipe->next;
                    }
                break;
                case SWIPE_OFF:
                default:
                    prev = &swipe->next;
                break;
            }
            swipe = *prev;
        }
    }
    state->last_t = t;
}

static color_t pixel(slot_t* slot, float x, float y) {
    state_t * state = (state_t *) slot->state;
    color_t output = state->color;
    float a = 0.;

    // TEMPLATE: Compute color at (x, y) from state
    for(int i = 0; i < N_SWIPE_SOURCES; i++){
        struct swipe * swipe = state->active_swipes[i];
        while(swipe){
            if(swipe->state != SWIPE_OFF){
                double fuzz = MIN(swipe->length / 2, state->sharp);
                double v = (x - swipe->ox) * swipe->kx + (y - swipe->oy) * swipe->ky;
                if(v > swipe->length)
                    a += 0;
                else if(v > (swipe->length - fuzz))
                    a += swipe->alpha * (swipe->length - v) / fuzz;
                else if(v > 0)
                    a += swipe->alpha;
                else if(v > -fuzz)
                    a += swipe->alpha * (fuzz + v) / fuzz;
                else
                    a += 0;
            }

            swipe = swipe->next;
        }
    }
    output.a = MIN(a, 1.0);
    return output;
}

static int event(slot_t* slot, enum pat_event event, float event_data){
    state_t * state = (state_t *) slot->state;
    if(isnan(event_data)) event_data = 0;
    struct swipe * swipe;
    switch(event){
        case PATEV_M1_NOTE_ON:
            // Pop off swipe from the front of inactive_swipes
            swipe = state->inactive_swipes;
            if(!swipe) return 0;
            if(swipe->next) state->inactive_swipes = swipe->next;
            // Prepend swipe onto active_swipes
            swipe->next = state->active_swipes[0];
            state->active_swipes[0] = swipe;

            swipe->ox = -1;
            swipe->oy = 0;
            swipe->length = 0;
            swipe->kx = 1;
            swipe->ky = 0;
            swipe->state = SWIPE_GROWING;
            swipe->alpha = event_data;
        break;
        case PATEV_M2_NOTE_ON:
            swipe = state->inactive_swipes;
            if(!swipe) return 0;
            if(swipe->next) state->inactive_swipes = swipe->next;
            state->inactive_swipes = swipe->next;
            swipe->next = state->active_swipes[1];
            state->active_swipes[1] = swipe;

            swipe->ox = 0;
            swipe->oy = -1;
            swipe->length = 0;
            swipe->kx = 0;
            swipe->ky = 1;
            swipe->state = SWIPE_GROWING;
            swipe->alpha = event_data;
        break;
        case PATEV_MOUSE_DOWN_X:
            swipe = state->inactive_swipes;
            if(!swipe) return 0;
            if(swipe->next) state->inactive_swipes = swipe->next;
            state->inactive_swipes = swipe->next;
            swipe->next = state->active_swipes[2];
            state->active_swipes[2] = swipe;

            swipe->ox = -1;
            swipe->oy = -1;
            swipe->length = 0;
            swipe->kx = .707;
            swipe->ky = .707;
            swipe->state = SWIPE_GROWING;
            swipe->alpha = 1.;
        break;
        case PATEV_M1_NOTE_OFF:
            if(!state->active_swipes[0]) break;
            state->active_swipes[0]->state = SWIPE_SHRINKING;
        break;
        case PATEV_M2_NOTE_OFF:
            if(!state->active_swipes[1]) break;
            state->active_swipes[1]->state = SWIPE_SHRINKING;
        break;
        case PATEV_MOUSE_UP_X:
            if(!state->active_swipes[2]) break;
            state->active_swipes[2]->state = SWIPE_SHRINKING;
        break;
        default:
        return 0;
    }
    return 1;
}

pattern_t pat_swipe = {
    .render = &pixel,
    .init = &init,
    .del = &del,
    .update = &update,
    .event = &event,
    .n_params = N_PARAMS,
    .parameters = params,
    .name = "Swipe",
};
