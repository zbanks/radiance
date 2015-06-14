#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_framerate.h>

#include "core/err.h"
#include "core/config.h"
#include "core/slot.h"
#include "core/state.h"
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

static void ui_done_callback()
{
    output_stop();
}

void catch_sigterm(int sig)
{
    UNUSED(sig);
    output_stop();
}

int main()
{
    colormap_test_all();
    colormap_set_global(&cm_rainbow_edged);
    colormap_set_mono(0.5);

    slots_init();
    pattern_init();
    patterns_updating = SDL_CreateMutex();

    //config_load(&config, "config.ini");
    //layout_load(&layout, config.path.layout);
    
    config_dump(&config, "config.ini");
    layout_dump(&layout, config.path.layout);
    
    /*
     * fade > 100fps
     * full > 100fps
     * strobe > 100fps
     * sparkle ~ 50fps
     * bubble ~42fps
     * rainbow ~ 38fps
     * wave ~ 35fps
     * swipe ~ 25fps
     */

    /*
    pattern_t * test_pat = &pat_sparkle;
    for(int i = 0; i < n_slots; i++){
        pat_load(&slots[i], test_pat);
    }
    */

    pat_load(&slots[0], &pat_rainbow);
    pat_load(&slots[1], &pat_fade);
    pat_load(&slots[2], &pat_strobe);
    pat_load(&slots[3], &pat_bubble);
    pat_load(&slots[4], &pat_strobe);
    pat_load(&slots[5], &pat_bubble);
    pat_load(&slots[6], &pat_swipe);
    pat_load(&slots[7], &pat_strobe);

    for(int i = 0; i < 8; i++){
        param_state_setq(&slots[i].param_states[0], (rand() % 1000) / 1000.);
    }

    filters_load();

    if (SDL_Init(config.ui.enabled ? SDL_INIT_VIDEO : 0))
    {
        FAIL("SDL_Init Error: %s\n", SDL_GetError());
    }

    // Set up signal handling before threads are made
    signal(SIGTERM, catch_sigterm);
    signal(SIGINT, catch_sigterm);

    audio_start();
    midi_start();
    signal_start();

    //state_load("state_0.ini");

    if(config.ui.enabled) ui_start(&ui_done_callback);

    output_init();
    output_run();

    printf("\nShutting down...\n");

    if(config.ui.enabled) ui_stop();
    signal_stop();
    midi_stop();
    audio_stop();

    filters_unload();

    SDL_DestroyMutex(patterns_updating);

    pattern_del();
    slots_del();

    return 0;
}

