#include "filters/filter.h"
#include "filters/audio.h"

#define N_FILTERS 1

void filter_onset_init(filter_t * filter){

}

void filter_onset_del(filter_t * filter){

}

void filter_onset_update(filter_t * filter, float t, chunk_t chunk){
    //audio_history(&filter->output.value, 1);
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
    .del = filter_onset_del,
    },
};

filter_t beat_filter = {
    .n_params = 0,
    .name = "Beat Detection",
    .output = { //
        .value = 0.0,
        .handle_color = {0, 0, 0},
        .label_color = {0, 0, 0},
        .label = "Beat?",
    },
    .init = filter_beat_init,
    .update = filter_beat_update,
    .del = filter_beat_del,
};

void update_filters(chunk_t chunk){
    beat_filter.update(&beat_filter, chunk);
    for(int i = 0; i < n_filters; i++){
        filters[i].update(&filters[i], chunk);
    }
}
