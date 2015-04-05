#include <SDL/SDL_thread.h>

#include "core/err.h"
#include "core/slot.h"
#include "output/slice.h"

output_vertex_t s1v2 = {
    .x = 0.3,
    .y = 0.7,
    .index = 199,
    .next = 0,
};

output_vertex_t s1v1 = {
    .x = 0.3,
    .y = 0.3,
    .index = 0,
    .next = &s1v2,
};

output_vertex_t s2v3 = {
    .x = 0.5,
    .y = 0.7,
    .index = 199,
    .next = 0,
};

output_vertex_t s2v2 = {
    .x = 0.7,
    .y = 0.7,
    .index = 99,
    .next = &s2v3,
};

output_vertex_t s2v1 = {
    .x = 0.7,
    .y = 0.3,
    .index = 0,
    .next = &s2v2,
};

#define N_OUTPUT_STRIPS 2

int n_output_strips = N_OUTPUT_STRIPS;

output_strip_t output_strips[N_OUTPUT_STRIPS] = {
    {
        .id = 0xFFFFFFFF,
        .length = 46,
        .first = &s1v1,
    },
    {
        .id = 2,
        .length = 20,
        .first = &s2v1,
    },
};

void output_to_buffer(output_strip_t* strip, color_t* buffer)
{
    if(SDL_LockMutex(patterns_updating)) FAIL("Unable to lock update mutex: %s\n", SDL_GetError());

    output_vertex_t* vert = strip->first;

    for(int i=0; i<strip->length; i++)
    {
        while(i > vert->next->index)
        {
            vert = vert->next;
            if(!vert->next) return; // Error condition
        }

        float alpha = (float)(i - vert->index) / (vert->next->index - vert->index);
        float x = alpha * vert->next->x + (1 - alpha) * vert->x;
        float y = alpha * vert->next->y + (1 - alpha) * vert->y;

        buffer[i] = render_composite(x, y);
    }

    SDL_UnlockMutex(patterns_updating);
}

