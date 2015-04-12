#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_framerate.h>

#include "core/err.h"
#include "core/slot.h"
#include "core/audio.h"
#include "filters/filter.h"
#include "output/output.h"
#include "output/slice.h"
#include "midi/midi.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "ui/ui.h"

#define MAX_FRAMERATE 30

int main()
{
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);
    pat_load(&slots[2], &pat_bubble);

    patterns_updating = SDL_CreateMutex();

    filters_load();

    output_start();
    audio_start();
    midi_start();
    signal_start();

    FPSmanager fps_manager;

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 60);

    for(;;)
    {
        int t = SDL_GetTicks();
        float tb = t / 1000.;

        if(ui_poll()) break;
        update_patterns(tb);
        update_signals(tb);
        ui_render();

        SDL_framerateDelay(&fps_manager);
    }

    signal_stop();
    midi_stop();
    audio_stop();
    output_stop();

    filters_unload();

    SDL_DestroyMutex(patterns_updating);

    ui_quit();

    return 0;
}

