#include <stdio.h>
#include "err.h"
#include "slot.h"
#include "pattern.h"
#include "ui.h"
#include <math.h>
#include <stdlib.h>
#include <slice.h>

int main()
{
    ui_init();

    pat_load(&slots[0], &pat_full);
    pat_load(&slots[1], &pat_wave);

    color_t buffer[200];

    for(;;)
    {
        float t = (float)SDL_GetTicks() / 1000.;

        if(ui_poll()) break;
        update_patterns(t);
        ui_render();
        //output_to_buffer(&output_strips[0], buffer);
        // TODO rate-limit
    }

    ui_quit();

    return 0;
}

