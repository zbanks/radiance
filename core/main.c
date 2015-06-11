#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_framerate.h>

#include "core/err.h"
#include "core/config.h"
#include "core/slot.h"
#include "core/audio.h"
#include "filters/filter.h"
#include "output/output.h"
#include "output/slice.h"
#include "midi/midi.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "signals/signal.h"
#include "timebase/timebase.h"
#include "util/color.h"
#include "ui/ui.h"
#include "ui/layout.h"

int main()
{
    colormap_test_all();
    colormap_set_global(&cm_rainbow);
    colormap_set_mono(0.5);

    pattern_init();
    patterns_updating = SDL_CreateMutex();

    config_dump(&config, "config.ini");
    layout_dump(&layout, config.path.layout);
    //return 0;
    //layout_load(&layout, "layout.ini");
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_fade);
    pat_load(&slots[2], &pat_bubble);
    pat_load(&slots[3], &pat_wave);
    pat_load(&slots[4], &pat_strobe);
    pat_load(&slots[5], &pat_wave);
    pat_load(&slots[6], &pat_bubble);
    pat_load(&slots[7], &pat_strobe);

    for(int i = 0; i < 8; i++){
        param_state_setq(&slots[i].param_states[0], (rand() % 1000) / 1000.);
    }


    filters_load();

    audio_start();
    midi_start();
    signal_start();
    output_start();

    FPSmanager fps_manager;

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 100);

    while(ui_poll())
    {
        //float tb = (float)timebase_get() / 1000; // TODO make all times long
        /*
        mbeat_t tb = timebase_get();

        update_patterns(tb);
        update_signals(tb);
        */

        ui_render();

        stat_fps = 1000. / SDL_framerateDelay(&fps_manager);
    }

    signal_stop();
    midi_stop();
    audio_stop();
    output_stop();

    filters_unload();

    SDL_DestroyMutex(patterns_updating);

    ui_quit();
    pattern_del();

    return 0;
}

