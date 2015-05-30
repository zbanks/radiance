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
#include "output/output.h"
#include "output/serial.h"
#include "output/slice.h"

#include <flux.h>
#include <czmq.h>

#define BROKER_URL "tcp://localhost:5555"

color_t** output_buffers = 0;

static int output_running;
static SDL_Thread* output_thread;

static flux_cli_t * flux_client;

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
        update_hits(tb);
        update_signals(tb);

        for(int i=0; i<n_output_strips; i++)
        {
            if(output_strips[i].bus < 0)
                continue;

            output_to_buffer(&output_strips[i], output_buffers[i]);

            float energy = 0.;
            float scalar = 1.0;
            for(int k = 0; k < output_strips[i].length; k++){
                energy += output_buffers[i][k].r;
                energy += output_buffers[i][k].g;
                energy += output_buffers[i][k].b;
            }
            scalar = output_strips[i].length * 2.0 / energy * 255.;
            if(scalar > 255.)
                scalar = 255.;

            int j = 0;
            for(int k = 0; k < output_strips[i].length; k++){
                frame[j++] = output_buffers[i][k].r * scalar;
                frame[j++] = output_buffers[i][k].g * scalar;
                frame[j++] = output_buffers[i][k].b * scalar;
            }

            zmsg_t * fmsg = zmsg_new();
            zmsg_t * reply = zmsg_new();
            zmsg_pushmem(fmsg, frame, j);
            flux_cli_send(flux_client, output_strips[i].id, "FRAME", &fmsg, &reply);
            zmsg_destroy(&reply);
        }
        stat_ops = 1000. / SDL_framerateDelay(&fps_manager);
    }
    return 0;
}

void output_start()
{
    output_buffers = malloc(sizeof(color_t*) * n_output_strips);

    if(!output_buffers) FAIL("Could not allocate output buffer array");

    flux_client = flux_cli_init(BROKER_URL, 0);
    if(flux_client){
        printf("Flux connected\n");
        for(int i = 0; i < n_output_strips; i++){
            output_strips[i].bus = flux_cli_id_check(flux_client, output_strips[i].id);
            if(output_strips[i].bus) continue;

            zmsg_t * imsg = zmsg_new();
            zmsg_t * reply = NULL;
            if(!flux_cli_send(flux_client, output_strips[i].id, "INFO", &imsg, &reply)){
                zhash_t * info = zhash_unpack(zmsg_first(reply));
                char * length_str  = zhash_lookup(info, "length");
                if(length_str)
                    output_strips[i].length = atoi(length_str);
                printf("Lux device '%s' with length '%s'\n", (char *) zhash_lookup(info, "id"), (char *) zhash_lookup(info, "length"));
                zhash_destroy(&info);
            }else{
                output_strips[i].bus = -1;
            }
            zmsg_destroy(&reply);
        }
    }else{
        printf("No flux initialized\n");
        for(int i = 0; i < n_output_strips; i++){
            output_strips[i].bus = -1;
        }
    }

    for(int i=0; i<n_output_strips; i++)
    {
        output_buffers[i] = malloc(sizeof(color_t) * output_strips[i].length);
        if(!output_buffers[i]) FAIL("Could not allocate output buffer");
    }

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

    serial_close();
}

