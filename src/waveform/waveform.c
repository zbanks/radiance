#include "waveform/waveform.h"
#include "timebase/timebase.h"
#include "audio/audio.h"
#include "core/time.h"
#include "core/config.h"
#include "ui/graph.h"

#include <math.h>
#include <string.h>
#include <SDL/SDL.h>

#define WF_LOW_COLOR {10, 10, 60, 255} // dark blue
#define WF_MID_COLOR {30, 40, 110, 255} // blue
#define WF_HIGH_COLOR {40, 90, 130, 255} // light blue
#define WF_LABEL_COLOR {30, 40, 110, 255}

struct waveform_bin waveform_bins[N_WF_BINS] = {
    [WF_LOW] = {
        .color = WF_LOW_COLOR,
        .output = {
            .value = 0.,
            .handle_color = WF_LABEL_COLOR,
            .label_color = WF_LABEL_COLOR,
            .label = "Lows",
            .connected_state = 0,
        },
    },
    [WF_MID] = {
        .color = WF_MID_COLOR,
        .output = {
            .value = 0.,
            .handle_color = WF_LABEL_COLOR,
            .label_color = WF_LABEL_COLOR,
            .label = "Mids",
            .connected_state = 0,
        },
    },
    [WF_HIGH] = {
        .color = WF_HIGH_COLOR,
        .output = {
            .value = 0.,
            .handle_color = WF_LABEL_COLOR,
            .label_color = WF_LABEL_COLOR,
            .label = "Highs",
            .connected_state = 0,
        },
    },
};

struct waveform_bin beat_bin = {
    .color = {255, 0, 0, 255},
};

void waveform_init(){
    for(int i = 0; i < N_WF_BINS; i++){
        memset(waveform_bins[i].history, 0, sizeof(WAVEFORM_HISTORY_SIZE * sizeof(float)));
        waveform_bins[i].hptr = 0;
        param_output_init(&waveform_bins[i].output, 0.);
    }
    memset(beat_bin.history, 0, sizeof(WAVEFORM_HISTORY_SIZE * sizeof(float)));
    beat_bin.hptr = 0;
}

static inline void waveform_bin_update(struct waveform_bin * bin, float value){
    if(bin->hptr >= WAVEFORM_HISTORY_SIZE)
        bin->hptr = 0;
    bin->history[bin->hptr++] = value;
}

void waveform_add_beatline(){
    beat_lines[0] |= 1;
}

void waveform_update(const chunk_pt chunk){
    //const float alpha = 0.98;

    float vall = 0.;
    float vhigh = 0.;
    float vmid = 0.;
    float vlow = 0.;

    static float slow = 0.;
    static float shigh = 0.;

    static float qlow = 0.;
    static float qmid = 0.;
    static float qhigh = 0.;

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define LB1 0.03
#define LB2 0.0
//#define LA2 0.9914878835315175  // exp(-pi * f_1 / F_n); F_n = 22050Hz; f_1 = 60Hz
//#define LA2 0.9719069870254697  // exp(-pi * f_1 / F_n); F_n = 22050Hz; f_1 = 200Hz
#define LA2 0.98
#define L2A2 0.98 // DC-blocking 1-pole lpf
#define LN  ((1. - LA2) / (LB1 + LB2)) 
#define HB1 1.0
#define HB2 -1.0
//#define HA2 0.6521846367685737 // exp(-pi * f_1 / F_n); F_n = 22050Hz; f_1 = 3000Hz
#define HA2 0.65 // exp(-pi * f_1 / F_n); F_n = 22050Hz; f_1 = 3000Hz
#define HN  ((1. + LA2) / 2.) * 4
#define H2A2 0.7 // DC-blocking 1-pole lpf

#define LC1 0.9
#define MC1 0.8
#define HC1 0.7


    for(int i = 0; i < config.audio.chunk_size; i++){
        vall = MAX(vall, fabs(chunk[i]));

        //slow = slow * alpha + chunk[i] * (1 - alpha);
        //vlow = MAX(vlow, fabs(slow));
       
        //vlow = MAX(vlow, fabs( (chunk[i] - slow * LA2) * LB1 + slow * LB2) * LN);
        //slow = chunk[i] - slow * LA2;

        slow = LA2 * slow + (1. - LA2) * chunk[i];
        //vlow = MAX(vlow, fabs(slow));
        vlow = L2A2 * vlow + (1.0 - L2A2) * fabs(slow);

        //shigh = - shigh * alpha + chunk[i] * (1 - alpha);
        //vhigh = MAX(vhigh, fabs(shigh));
        
        //vhigh = MAX(vhigh, fabs( (chunk[i] - shigh * HA2) * HB1 + shigh * HB2) * HN);
        vhigh = H2A2 * vhigh + (1.0 - H2A2) * fabs(((chunk[i] - shigh * HA2) * HB1 + shigh * HB2) * HN);
        shigh = chunk[i] - shigh * HA2;
    }
    vhigh *= 1.5;
    vlow *= 2.0;
    vhigh = MIN(vhigh, vall);
    vlow = MIN(vlow, vall);
    vmid = MAX(0., vall - vlow - vhigh);

    waveform_bin_update(&waveform_bins[WF_LOW], (vlow + vmid + vhigh));
    waveform_bin_update(&waveform_bins[WF_MID], (vhigh + vmid));
    waveform_bin_update(&waveform_bins[WF_HIGH], (vhigh));
    waveform_bin_update(&beat_bin, MB2B(timebase_get() % 1000));

    qlow = LC1 * qlow + (1. - LC1) * vlow;
    qmid = MC1 * qmid + (1. - MC1) * vmid;
    qhigh = HC1 * qhigh + (1. - HC1) * vhigh;

    param_output_set(&waveform_bins[WF_LOW].output, MIN(qlow * 2., 1.));
    param_output_set(&waveform_bins[WF_MID].output, MIN(qmid * 2., 1.));
    param_output_set(&waveform_bins[WF_HIGH].output, MIN(qhigh * 2., 1.));

    memmove(beat_lines + 1, beat_lines, (WAVEFORM_HISTORY_SIZE - 1) * sizeof(char));
    beat_lines[0] = 0;
}

void waveform_del(){
}

