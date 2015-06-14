#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>
#include <SDL/SDL_framerate.h>

#include "core/err.h"
#include "core/slot.h"
#include "signals/signal.h"
#include "timebase/timebase.h"
#include "core/time.h"
#include "core/config.h"
#include "output/output.h"
#include "output/slice.h"
#include "output/flux.h"
#include "output/lux.h"

color_t** output_buffers = 0;

static int output_running;
static SDL_Thread* output_thread;

static int output_on_flux = 0;
static int output_on_lux = 0;

static int output_run(void* args)
{
    UNUSED(args);
    FPSmanager fps_manager;
    unsigned char frame[4096];

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 100);
    
    while(output_running)
    {
        mbeat_t tb = timebase_get();

        update_patterns(tb);
        update_signals(tb);

        for(int i=0; i<n_output_strips; i++)
        {
            if(!output_strips[i].bus)
                continue;

            output_to_buffer(&output_strips[i], output_buffers[i]);

            float energy = 0.;
            float scalar = 1.0;
            for(int k = 0; k < output_strips[i].length; k++){
                energy += output_buffers[i][k].r;
                energy += output_buffers[i][k].g;
                energy += output_buffers[i][k].b;
            }
            scalar = output_strips[i].length * config.output.max_energy / energy * 255.;
            if(scalar > 255.)
                scalar = 255.;

            int j = 0;
            for(int k = 0; k < output_strips[i].length; k++){
                frame[j++] = output_buffers[i][k].r * scalar;
                frame[j++] = output_buffers[i][k].g * scalar;
                frame[j++] = output_buffers[i][k].b * scalar;
            }

            if(output_on_flux && (output_strips[i].bus & OUTPUT_FLUX))
                output_flux_push(&output_strips[i], frame, j);
            if(output_on_lux && (output_strips[i].bus & OUTPUT_LUX))
                output_lux_push(&output_strips[i], frame, j);
        }
        SDL_framerateDelay(&fps_manager);
        stat_ops = SDL_getFramerate(&fps_manager);
    }
    return 0;
}

void output_start()
{
    output_buffers = malloc(sizeof(color_t*) * n_output_strips);

    if(!output_buffers) FAIL("Could not allocate output buffer array");

    for(int i=0; i<n_output_strips; i++)
    {
        output_buffers[i] = malloc(sizeof(color_t) * output_strips[i].length);
        if(!output_buffers[i]) FAIL("Could not allocate output buffer");

        // TODO: better unify lux/flux 
        sprintf(output_strips[i].id_str, "lux:%08x", output_strips[i].id_int);
    }

    output_on_flux = !output_flux_init();
    output_on_lux = !output_lux_init();

    if(!(output_on_flux || output_on_lux)){
        printf("No flux or lux initialized\n");
        for(int i = 0; i < n_output_strips; i++){
            output_strips[i].bus = -1;
        }
    }

    int n_flux = 0;
    int n_lux = 0;

    if(output_on_flux) n_flux = output_flux_enumerate(output_strips, n_output_strips);
    if(output_on_lux) n_lux = output_lux_enumerate(output_strips, n_output_strips);

    printf("Found %d flux devices & %d lux devices\n", n_flux, n_lux);

    // Create output thread to run updates even if no serial was init'd
    output_running = 1;
    output_thread = SDL_CreateThread(&output_run, 0);
    if(!output_thread) FAIL("Could not create output thread: %s\n",SDL_GetError());
}

void output_stop()
{
    output_running = 0;

    SDL_WaitThread(output_thread, 0);

    for(int i=0; i<n_output_strips; i++)
    {
        free(output_buffers[i]);
    }

    free(output_buffers);

    if(output_on_flux)
        output_flux_del();
    if(output_on_lux)
        output_lux_del();
}

