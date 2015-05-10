#include "core/parameter.h"
#include "filters/filter.h"
#include "filters/vamp.h"
#include "timebase/timebase.h"

#define N_FILTERS 2

int n_filtered_chunks = 0;

struct filter_lpf_agc_state {
    double agc_scale;
    double agc_alpha;
    double lpf_last;
    double lpf_alpha;
};

void filter_diodelpf_agc_update(filter_t * filter, mbeat_t t_msec, double value){
    struct filter_lpf_agc_state * state = filter->state;

    state->agc_scale *= state->agc_alpha;
    if(value > state->lpf_last) 
        state->lpf_last = value;
    else 
        value = state->lpf_last = (state->lpf_alpha * value) + ((1 - state->lpf_alpha) * state->lpf_last);

    if(value > state->agc_scale) state->agc_scale = value;

    param_output_set(&filter->output, value / state->agc_scale);
}

void filter_lpf_agc_update(filter_t * filter, mbeat_t t_msec, double value){
    struct filter_lpf_agc_state * state = filter->state;

    state->agc_scale *= state->agc_alpha;
    /*
    if(value > state->lpf_last) 
        state->lpf_last = value;
    else 
    */
    value = state->lpf_last = (state->lpf_alpha * value) + ((1 - state->lpf_alpha) * state->lpf_last);

    if(value > state->agc_scale) state->agc_scale = value;

    param_output_set(&filter->output, value / state->agc_scale);
}

void filter_beat_update(filter_t * filter, mbeat_t t_msec, double value)
{
    timebase_tap();
    //printf("Beat: %d\n", t_msec);
}


int n_filters = N_FILTERS;
filter_t filters[N_FILTERS] = {
    {
    .n_params = 0,
    .parameters = 0,
    .name = "Onset Detection",
    .color = {255, 0, 255},
    .output = {
            .value = 0.0,
            .handle_color = {255, 0, 255},
            .label_color = {255, 0, 255},
            .label = "ODF",
        },
    .init = 0,
    .update = filter_diodelpf_agc_update,
    .del = 0,
    .state = &((struct filter_lpf_agc_state) {
        .agc_scale = 512.,
        .agc_alpha = 0.999,
        .lpf_last = 0.,
        .lpf_alpha = 0.05,
    }),
    .vamp_so = "qm-vamp-plugins.so",
    .vamp_id = "qm-onsetdetector",
    .vamp_output = 1,
    },
    {
    .n_params = 0,
    .parameters = 0,
    .name = "Fundamental Freq",
    .color = {128, 0, 255},
    .output = {
            .value = 0.0,
            .handle_color = {128, 0, 255},
            .label_color = {128, 0, 255},
            .label = "F0",
        },
    .init = 0,
    .update = filter_lpf_agc_update,
    .del = 0,
    .state = &((struct filter_lpf_agc_state) {
        .agc_scale = 128.,
        .agc_alpha = 0.999999,
        .lpf_last = 0.,
        .lpf_alpha = 0.05,
    }),
    .vamp_so = "pyin.so",
    .vamp_id = "pyin",
    .vamp_output = 0,
    },
};

filter_t beat_filter = {
    .n_params = 0,
    .parameters = 0,
    .name = "Beat Detection",
    .output = { //
        .value = 0.0,
        .handle_color = {0, 0, 0},
        .label_color = {0, 0, 0},
        .label = "Beat?",
    },
    .init = 0,
    .update = filter_beat_update,
    .del = 0,
    .vamp_so = "btrack.so",
    .vamp_id = "btrack-vamp",
    .vamp_output = 0,
};

void filters_load(){
    n_filtered_chunks = 0;
    vamp_plugin_load(&beat_filter);
    if(beat_filter.init)
        beat_filter.init(&beat_filter);
    for(int i = 0; i < n_filters; i++){
        if(vamp_plugin_load(&filters[i])){
            printf("Error initializing filter '%s'\n", filters[i].name);
        }else{
            printf("Initializing filter '%s'\n", filters[i].name);
            if(filters[i].init)
                filters[i].init(&filters[i]);
            graph_create_filter(&filters[i].graph_state);
        }
    }
}

void filters_unload(){
    vamp_plugin_unload(&beat_filter);
    if(beat_filter.del)
        beat_filter.del(&beat_filter);
    for(int i = 0; i < n_filters; i++){
        vamp_plugin_unload(&filters[i]);
        if(filters[i].del)
            filters[i].del(&filters[i]);
    }
}

void filters_update(chunk_pt chunk){
    vamp_plugin_update(&beat_filter, chunk);
    for(int i = 0; i < n_filters; i++){
        vamp_plugin_update(&filters[i], chunk);
    }
    n_filtered_chunks++;
}
