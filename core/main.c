#include <stdio.h>
#include "err.h"
#include "slot.h"
#include "pattern.h"
#include "ui.h"
#include <math.h>
#include <stdlib.h>

int main()
{
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);

    for(;;)
    {
        float t = (float)SDL_GetTicks() / 1000.;

        if(ui_poll()) break;
        update_patterns(t);
        ui_render();
        // TODO rate-limit
    }

    pat_unload(&slots[0]);
    pat_unload(&slots[1]);
    ui_quit();

    return 0;
}

