#include "core/parameter.h"
#include "core/config.h"
#include "core/err.h"
#include "filters/filter.h"
#include "filters/vamp.h"
#include "timebase/timebase.h"
#include "util/signal.h"
#include "waveform/waveform.h"

#define N_FILTERS 3

int n_filtered_chunks = 0;

struct filter_lpf_agc_state {
    struct agc_state agc_state;
    struct ema_state dema_state;

    // The following get populated into the previous fields on init
    // If memory were actually a concern, we could union over the previous fields
    // ...but it's not.
    double agc_tau;
    double agc_knee_high;
    double agc_knee_low;
    double dema_tau_rise;
    double dema_tau_fall;
};

void filter_lpf_agc_update(filter_t * filter, mbeat_t t, double value){
    struct filter_lpf_agc_state * state = filter->state;

    double value_scaled = agc_update(&state->agc_state, t, value);
    double value_lpf = dema_update(&state->dema_state, t, value_scaled);

    param_output_set(&filter->output, (float) value_lpf);
}

void filter_lpf_agc_init(filter_t * filter){
    struct filter_lpf_agc_state * state = filter->state;
    agc_init(&state->agc_state, 1.0, 0.0, state->agc_knee_high, state->agc_knee_low, state->agc_tau);
    dema_init(&state->dema_state, state->dema_tau_rise, state->dema_tau_fall);
}

void filter_beat_update(filter_t * filter, mbeat_t t_msec, double value)
{

    if(timebase_source == TB_AUTOMATIC){
        timebase_tap(config.timebase.beat_btrack_alpha);
    }
    waveform_add_beatline();
}


int n_filters = N_FILTERS;
filter_t filters[N_FILTERS] = {
    {
    .enabled = 1,
    .n_params = 0,
    .parameters = 0,
    .name = "Onset Detection",
    .display = 1,
    .color = {255, 0, 255, 255},
    .output = {
            .value = 0.0,
            .handle_color = {255, 0, 255, 255},
            .label_color = {255, 0, 255, 255},
            .label = "ODF",
        },
    .init = filter_lpf_agc_init,
    .update = filter_lpf_agc_update,
    .del = 0,
    .state = &((struct filter_lpf_agc_state) {
        .agc_tau = 1.,
        .agc_knee_high = 400.,
        .agc_knee_low = 100.,
        .dema_tau_rise = 0.003,
        .dema_tau_fall = 0.22,
    }),
    .vamp_so = "qm-vamp-plugins.so",
    .vamp_id = "qm-onsetdetector",
    .vamp_output = 1,
    },
    {
    .enabled = 1,
    .n_params = 0,
    .parameters = 0,
    .name = "Fundamental Freq",
    .display = 1,
    .color = {128, 0, 255, 0},
    .output = {
            .value = 0.0,
            .handle_color = {128, 0, 255, 255},
            .label_color = {128, 0, 255, 255},
            .label = "F0",
        },
    .init = filter_lpf_agc_init,
    .update = filter_lpf_agc_update,
    .del = 0,
    .state = &((struct filter_lpf_agc_state) {
        .agc_tau = 3.,
        .agc_knee_high = 128.,
        .agc_knee_low = 0.,
        .dema_tau_rise = 0.07,
        .dema_tau_fall = 0.07,
    }),
    .vamp_so = "pyin.so",
    .vamp_id = "pyin",
    .vamp_output = 0,
    },
    { // Beat Filter
    .enabled = 0,
    .n_params = 0,
    .parameters = 0,
    .name = "Beat Detection",
    .display = 0,
    .output = { //
        .handle_color = {0, 0, 0, 255},
        .label_color = {0, 0, 0, 255},
        .label = "Beat?",
    },
    .init = 0,
    .update = filter_beat_update,
    .del = 0,
    .vamp_so = "btrack.so",
    .vamp_id = "btrack-vamp",
    .vamp_output = 0,
    },

};

void filters_load(){
    n_filtered_chunks = 0;
    for(int i = 0; i < n_filters; i++){
        if(!filters[i].enabled) continue;
        if(vamp_plugin_load(&filters[i])){
            printf("Error initializing filter '%s'\n", filters[i].name);
        }else{
            printf("Initializing filter '%s'\n", filters[i].name);
            if(filters[i].display)
                graph_create_filter(&filters[i].graph_state);
            param_output_init(&filters[i].output, 0.);
            if(filters[i].init)
                filters[i].init(&filters[i]);
        }
    }
}

void filters_unload(){
    for(int i = 0; i < n_filters; i++){
        if(!filters[i].enabled) continue;
        vamp_plugin_unload(&filters[i]);
        if(filters[i].del)
            filters[i].del(&filters[i]);
        param_output_free(&filters[i].output);
        graph_remove(&filters[i].graph_state);
    }
}

void filters_update(const chunk_pt chunk){
    for(int i = 0; i < n_filters; i++){
        if(!filters[i].enabled) continue;
        vamp_plugin_update(&filters[i], chunk);
    }
    n_filtered_chunks++;
}

