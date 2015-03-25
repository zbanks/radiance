#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <SDL/SDL_thread.h>
#include "err.h"
#include "slot.h"
#include "pattern.h"
#include "ui.h"
#include "slice.h"

int main()
{
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);

    patterns_updating = SDL_CreateMutex();

    output_start();

    for(;;)
    {
        float t = (float)SDL_GetTicks() / 1000.;

        if(ui_poll()) break;
        update_patterns(t);
        ui_render();
        // TODO rate-limit
    }

    output_stop();

    SDL_DestroyMutex(patterns_updating);

    ui_quit();

    return 0;
}

