#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>
#include "audio.h"
#include "err.h"
#include "output.h"
#include "pattern.h"
#include "slice.h"
#include "slot.h"
#include "ui.h"

int main()
{
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);
    pat_load(&slots[2], &pat_bubble);

    patterns_updating = SDL_CreateMutex();

    output_start();
    audio_start();
    midi_start();
    input_start();

    for(;;)
    {
        float t = (float)SDL_GetTicks() / 1000.;

        if(ui_poll()) break;
        update_patterns(t);
        update_inputs(t);
        ui_render();
        // TODO rate-limit
    }

    input_stop();
    midi_stop();
    audio_stop();
    output_stop();

    SDL_DestroyMutex(patterns_updating);

    ui_quit();

    return 0;
}

