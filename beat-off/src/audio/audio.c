#include "core/err.h"
#include "core/config.h"
#include "audio/audio.h"
#include "audio/input_pa.h"
#include "filters/filter.h"
#include "waveform/waveform.h"
#include "timebase/timebase.h"
#include "BTrack.h"

#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static volatile int audio_running;
static SDL_Thread* audio_thread;
static struct btrack btrack;
static double * double_chunk;

static int audio_callback(chunk_pt chunk){
#ifdef VAMP_ENABLED
    filters_update(chunk);
#endif
    waveform_update(chunk);

    // Convert chunk (float[]) to an array of doubles
    for(int i = 0; i < config.audio.chunk_size; i++){
        double_chunk[i] = *chunk++;
    }

    btrack_process_audio_frame(&btrack, double_chunk);

    if(btrack_beat_due_in_current_frame(&btrack)){
        if(timebase_source == TB_AUTOMATIC){
            timebase_tap(config.timebase.beat_btrack_alpha);
        }
        waveform_add_beatline();
    }

    if(!audio_running) return -1;
    return 0;
}

static int audio_run(void* args) {
    audio_pa_run(&audio_callback, config.audio.sample_rate, config.audio.chunk_size);

    if(audio_running) return -1;
    return 0;
}

void audio_start()
{
    btrack_init(&btrack, config.audio.chunk_size, config.audio.chunk_size);
    timebase_init();
    waveform_init();

    double_chunk = malloc(config.audio.chunk_size * sizeof(double));
    if(!double_chunk) FAIL("Could not malloc space for double chunk.\n");

    audio_running = 1;

    audio_thread = SDL_CreateThread(&audio_run, 0);
    if(!audio_thread) FAIL("Could not create output thread: %s\n",SDL_GetError());
}

void audio_stop()
{
    audio_running = 0;

    SDL_WaitThread(audio_thread, 0);

    waveform_del();
    timebase_del();
    btrack_del(&btrack);

    printf("Audio thread stopped.\n");
}

