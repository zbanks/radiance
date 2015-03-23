#include "output.h"
#define _BSD_SOURCE
#include <unistd.h>
#include <stdlib.h>

#include "slice.h"
#include "slot.h"
#include "err.h"

int output_running;

color_t** output_buffers = 0;

static pthread_t output_thread;

static pthread_attr_t attr;
 
static void* output_run(void* args)
{
    while(output_running)
    {
        for(int i=0; i<n_output_strips; i++)
        {
            output_to_buffer(&output_strips[i], output_buffers[i]);
        }
        usleep(1000);
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

    output_running = 1;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if(pthread_create(&output_thread, &attr, &output_run, 0)) FAIL("Could not create output thread");
}

void output_stop()
{
    output_running = 0;

    pthread_join(output_thread, 0);

    pthread_attr_destroy(&attr);

    for(int i=0; i<n_output_strips; i++)
    {
        free(output_buffers[i]);
    }

    free(output_buffers);
}


