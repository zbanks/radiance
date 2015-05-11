#include "waveform/waveform.h"
#include "core/audio.h"
#include "ui/graph.h"

#include <math.h>
#include <string.h>
#include <SDL/SDL.h>

struct waveform_bin waveform_bins[N_WF_BINS] = {
    [WF_HIGH] = {
        .color = {40, 90, 130}, // light blue
    },
    [WF_MID] = {
        .color = {30, 40, 60}, // blue
    },
    [WF_LOW] = {
        .color = {10, 10, 60}, // dark blue
    },
};

void waveform_init(){
    for(int i = 0; i < N_WF_BINS; i++){
        memset(waveform_bins[i].history, 0, sizeof(waveform_bins[i].history));
    }
}

static inline void waveform_history_update(float * history, float value){
    memmove(history + 1, history, (WAVEFORM_HISTORY_SIZE - 1) * sizeof(float));
    *history = value;
}

void waveform_update(chunk_pt chunk){
    const float alpha = 0.995;

    float vall = 0.;
    float vhigh = 0.;
    float vmid = 0.;
    float vlow = 0.;

    float slow = 0.;
    float shigh = 0.;

#define MAX(a, b) ((a > b) ? a : b)
    for(int i = 0; i < FRAMES_PER_BUFFER; i++){
        vall = MAX(vall, fabs(chunk[i]));

        slow = slow * alpha + chunk[i] * (1 - alpha);
        vlow = MAX(vlow, fabs(slow));

        shigh = shigh * alpha - chunk[i] * (1 - alpha);
        vhigh = MAX(vhigh, fabs(shigh));

    }

    vmid = MAX(0., vall - vlow - vhigh);

    waveform_history_update(waveform_bins[WF_HIGH].history, (vlow + vmid + vhigh));
    waveform_history_update(waveform_bins[WF_MID].history, (vlow + vmid));
    waveform_history_update(waveform_bins[WF_LOW].history, (vlow));
}

void waveform_del(){
}

