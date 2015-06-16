#include "core/err.h"
#include "core/config.h"
#include "audio/audio.h"
#include "audio/input_pa.h"
#include "filters/filter.h"
#include "waveform/waveform.h"
#include "timebase/timebase.h"

#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int audio_running;

static SDL_Thread* audio_thread;

static int audio_callback(chunk_pt chunk){
    filters_update(chunk);
    waveform_update(chunk);

    if(!audio_running) return -1;
    return 0;
}

static int audio_run(void* args) {
    UNUSED(args);
    audio_pa_run(&audio_callback, config.audio.sample_rate, config.audio.chunk_size);

    if(audio_running) return -1;
    return 0;
}

void audio_start()
{
    timebase_init();
    waveform_init();

    audio_running = 1;

    audio_thread = SDL_CreateThread(&audio_run, 0);
    if(!audio_thread) FAIL("Could not create output thread: %s\n",SDL_GetError());
}

void audio_stop()
{
    audio_running = 0;

    SDL_WaitThread(audio_thread, 0);

    timebase_del();
    waveform_del();
}

