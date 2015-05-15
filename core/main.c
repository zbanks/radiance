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
#include "timebase/timebase.h"
#include "ui/ui.h"
#include "ui/layout.h"

static float random_color()
{
    return (rand() % 1000) / 1000. ;
}

int main()
{
    patterns_updating = SDL_CreateMutex();
    hits_updating = SDL_CreateMutex();

    dump_layout(&layout, "layout.ini");
    //return 0;
    //load_layout(&layout, "layout.ini");
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);
    pat_load(&slots[2], &pat_bubble);
    pat_load(&slots[3], &pat_strobe);
    pat_load(&slots[4], &pat_full);
    pat_load(&slots[5], &pat_wave);
    pat_load(&slots[6], &pat_bubble);
    pat_load(&slots[7], &pat_strobe);

    hit_load(&hit_slots[0], &hit_full);
    hit_load(&hit_slots[1], &hit_full);
    hit_load(&hit_slots[2], &hit_full);
    hit_load(&hit_slots[3], &hit_pulse);
    hit_load(&hit_slots[4], &hit_pulse);
    hit_load(&hit_slots[5], &hit_pulse);
    hit_load(&hit_slots[6], &hit_circle);
    hit_load(&hit_slots[7], &hit_circle);
    for(int i = 0; i < 8; i++){
        param_state_setq(&hit_slots[i].alpha, 1.);
        param_state_setq(&slots[i].param_states[0], random_color());
        param_state_setq(&hit_slots[i].param_states[0], random_color());
    }


    filters_load();

    audio_start();
    midi_start();
    signal_start();
    output_start();

    FPSmanager fps_manager;

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 40);

    while(ui_poll())
    {
        //float tb = (float)timebase_get() / 1000; // TODO make all times long
        mbeat_t tb = timebase_get();

        update_patterns(tb);
        update_hits(tb);
        update_signals(tb);
        ui_render();

        stat_fps = 1000. / SDL_framerateDelay(&fps_manager);
    }

    signal_stop();
    midi_stop();
    audio_stop();
    output_stop();

    filters_unload();

    SDL_DestroyMutex(patterns_updating);
    SDL_DestroyMutex(hits_updating);

    ui_quit();

    return 0;
}

