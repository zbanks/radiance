#include "filters/filter.h"
#include "filters/audio.h"

#define N_FILTERS 1

#define N_ONSET_HISTORY 1024
float onset_history[N_ONSET_HISTORY];

void filter_onset_init(filter_t * filter){

}

void filter_onset_del(filter_t * filter){

}

void filter_onset_update(filter_t * filter, float t, chunk_t chunk){
    //audio_history(&filter->output.value, 1);
}

void filter_onset_history(filter_t * filter, float * values, int n_values){
    audio_history(values, n_values);
    param_output_set(&filter->output, values[0] / 400.);
    /*
    for(int i = 0; (i < n_values) && (i < N_ONSET_HISTORY); i++){
        values[i] = onset_history[N_ONSET_HISTORY - 1 - i];
    }
    */
}

int n_filters = N_FILTERS;
filter_t filters[N_FILTERS] = {
    {
    .n_params = 0,
    .name = "Onset Detection",
    .output = {
            .value = 0.0,
            .handle_color = {255, 0, 255},
            .label_color = {255, 0, 255},
            .label = "ODF"
        },
    .init = filter_onset_init,
    .update = filter_onset_update,
    .history = filter_onset_history,
    .del = filter_onset_del,
    },
};

void update_filters(float t, chunk_t chunk){
    for(int i = 0; i < n_filters; i++){
        filters[i].update(&filters[i], t, chunk);
    }
}
