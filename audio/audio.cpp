#include "util/common.h"
#include "util/err.h"
#include "util/config.h"
#include "audio/audio.h"
#include "audio/input_pa.h"
#include "audio/analyze.h"

#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
    std::atomic<bool> audio_running{false};
    SDL_Thread* audio_thread;
    //static struct btrack btrack;
    std::unique_ptr<double[]> double_chunk;

    int audio_callback(chunk_pt chunk) {
        analyze_chunk(chunk);

        // Convert chunk (float[]) to an array of doubles
        std::copy_n(chunk,config.audio.chunk_size, double_chunk.get());

        //btrack_process_audio_frame(&btrack, double_chunk);

        //if(btrack_beat_due_in_current_frame(&btrack)){
        //    if(timebase_source == TB_AUTOMATIC){
        //        timebase_tap(config.timebase.beat_btrack_alpha);
        //    }
        //    waveform_add_beatline();
        //}

        if(!audio_running)
            return -1;
        return 0;
    }

    int audio_run(void* args)
    {
        audio_pa_run(&audio_callback, config.audio.sample_rate, config.audio.chunk_size);
        if(audio_running)
            return -1;
        return 0;
    }
}
void audio_start()
{
    // btrack_init(&btrack, config.audio.chunk_size, config.audio.chunk_size);
    // timebase_init();
    // waveform_init();
    double_chunk = std::make_unique<double[]>(config.audio.chunk_size);
    audio_running = true;
    audio_thread = SDL_CreateThread(&audio_run, "Audio", 0);
    if(!audio_thread)
        FAIL("Could not create output thread: %s\n",SDL_GetError());
}

void audio_stop()
{
    audio_running = false;
    SDL_WaitThread(audio_thread, 0);
    //waveform_del();
    //timebase_del();
    //btrack_del(&btrack);
    INFO("Audio thread stopped.\n");
}

