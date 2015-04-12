#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_framerate.h>

#include "core/err.h"
#include "core/slot.h"
#include "core/audio.h"
#include "filters/filter.h"
#include "hits/hit.h"
#include "output/output.h"
#include "output/slice.h"
#include "midi/midi.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "ui/ui.h"

int main()
{
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);
    pat_load(&slots[2], &pat_bubble);

    hit_load(&hit_slots[0], &hit_full);

    patterns_updating = SDL_CreateMutex();

    filters_load();

    output_start();
    audio_start();
    midi_start();
    signal_start();

    FPSmanager fps_manager;

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 60);

    while(ui_poll())
    {
        float tb = (float)timebase_get() / 1000; // TODO make all times long

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

