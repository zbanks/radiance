#include "output.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "slice.h"
#include "slot.h"
#include "err.h"
#include "serial.h"
#include "crc.h"
#include "lux.h"
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

int output_running;

color_t** output_buffers = 0;

static SDL_Thread* output_thread;

static int output_run(void* args)
{
    struct lux_frame lf;
    char r;
    
    while(output_running)
    {
        for(int i=0; i<n_output_strips; i++)
        {
            output_to_buffer(&output_strips[i], output_buffers[i]);

            int j = 0;

            for(int k = 0; k < output_strips[i].length; k++){
                lf.data.carray.data[j++] = output_buffers[i][k].r * 20;
                lf.data.carray.data[j++] = output_buffers[i][k].g * 20;
                lf.data.carray.data[j++] = output_buffers[i][k].b * 20;
            }
            lf.data.carray.cmd = 0x90;// j++;
            lf.destination = output_strips[i].id;
            lf.length = j+1;

            if((r = lux_tx_packet(&lf)))
                printf("failed cmd: %d\n", r);
        }
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
    }

    if(serial_init()){
        printf("Serial initialized\n");
        output_running = 1;

        output_thread = SDL_CreateThread(&output_run, 0);
        if(!output_thread) FAIL("Could not create output thread: %s\n",SDL_GetError());
    }else{
        printf("No serial initialized\n");
    }
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

