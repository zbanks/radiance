#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_framerate.h>

#include "core/err.h"
#include "core/config.h"
#include "core/slot.h"
#include "state/state.h"
#include "audio/audio.h"
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

#include "util/perlin.h"

static void ui_done_callback()
{
    output_stop();
}

void catch_sigterm(int sig)
{
    output_stop();
}

int main()
{
    perlin_init();

    config_init(&config);
    config_load(&config, "config.ini");
    config_dump(&config, "config.ini"); // Dump parsed copy (remove "in production", useful because it adds new fields)

    layout_init(&layout);
    layout_load(&layout, config.path.layout);
    layout_dump(&layout, config.path.layout);

    colormap_test_all();
    colormap_set_global(&cm_rainbow_edged);
    colormap_set_mono(0.5);

    slots_init();
    pattern_init();
    patterns_updating = SDL_CreateMutex();

    
    /*
     * fade > 100fps
     * full > 100fps
     * strobe > 100fps
     * swipe ~ 70fps
     * sparkle ~ 62fps
     * rainbow ~ 52fps
     * bubble ~42fps
     * wave ~ 42fps
     */

    /*
    pattern_t * test_pat = &pat_wave;
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
        if(!slots[i].pattern) continue;
        for(int j = 0; j < slots[i].pattern->n_params; j++){
            if(strcmp(slots[i].pattern->parameters[j].name, "Color") == 0)
                param_state_setq(&slots[i].param_states[j], (rand() % 1000) / 1000.);
        }
    }


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


    if(config.ui.enabled) ui_start(&ui_done_callback);

#ifdef VAMP_ENABLED
    filters_load();
#endif
    //state_load("state_0.ini");

    output_init();
    output_run();

    printf("\nShutting down...\n");

    if(config.ui.enabled) ui_stop();
    signal_stop();
    midi_stop();
    audio_stop();

#ifdef VAMP_ENABLED
    filters_unload();
#endif

    SDL_DestroyMutex(patterns_updating);

    pattern_del();
    slots_del();
    config_del(&config);
    layout_del(&layout);

    return 0;
}

