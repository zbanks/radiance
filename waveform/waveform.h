#ifndef __FILTERS_AUDIO_H__
#define __FILTERS_AUDIO_H__

#include "core/audio.h"
#include "core/parameter.h"

#include <SDL/SDL.h>

#define WAVEFORM_HISTORY_SIZE 2048

struct waveform_bin {
    SDL_Color color;
    param_output_t output;
    float history[WAVEFORM_HISTORY_SIZE];
};

char beat_lines[WAVEFORM_HISTORY_SIZE];

enum freq_bin_i {
    WF_LOW,
    WF_MID,
    WF_HIGH,

    N_WF_BINS
};

extern struct waveform_bin waveform_bins[];
extern struct waveform_bin beat_bin;

void waveform_init();
void waveform_update(chunk_pt chunk);
void waveform_del();
void waveform_add_beatline();


#endif
