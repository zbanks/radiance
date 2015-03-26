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
    unsigned char x;
    while(output_running)
    {
        for(int i=0; i<n_output_strips; i++)
        {
            output_to_buffer(&output_strips[i], output_buffers[i]);

            lux_hal_disable_rx();
            lux_packet_in_memory = 0;

            *(uint32_t*)lux_destination = output_strips[i].id;

            int j = 0;
            lux_packet[j++] = 0x90;

            for(int k = 0; k < output_strips[i].length; k++){
                lux_packet[j++] = output_buffers[i][k].r * 20;
                lux_packet[j++] = output_buffers[i][k].g * 20;
                lux_packet[j++] = output_buffers[i][k].b * 20;
            }
            //lux_packet_length = 3 * output_strips[i].length + 1;
            lux_packet_length = j;
            lux_start_tx();
            for(int i = 0; i < 1500; i++) lux_codec(); //FIXME

            SDL_Delay(2);
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
}

