#include <SDL/SDL_thread.h>

#include "core/err.h"
#include "core/slot.h"
#include "output/slice.h"

#define SP 0.1

output_vertex_t s1v2 = {
    .x = -1.0,
    .y = SP,
    .index = 60,
    .next = 0,
};

output_vertex_t s1v1 = {
    .x = 0.9,
    .y = SP,
    .index = 0,
    .next = &s1v2,
};

output_vertex_t s2v2 = {
    .x = -1.0,
    .y = -SP,
    .index = 60,
    .next = 0,
};

output_vertex_t s2v1 = {
    .x = 0.9,
    .y = -SP,
    .index = 0,
    .next = &s2v2,
};

output_vertex_t s3v2 = {
    .x = 0.9,
    .y = 1.0,
    .index = 60,
    .next = 0,
};

output_vertex_t s3v1 = {
    .x = 0.9,
    .y = -0.45,
    .index = 0,
    .next = &s3v2,
};

#define N_OUTPUT_STRIPS 4

int n_output_strips = N_OUTPUT_STRIPS;

output_strip_t output_strips[N_OUTPUT_STRIPS] = {
    {
        .id_int = 0x00000001,
        .length = 60,
        .first = &s1v1,
        .color = {255,255,0, 255},
    },
    {
        .id_int = 0x00000002,
        .length = 60,
        .first = &s2v1,
        .color = {255,150,0, 255},
    },
    {
        .id_int = 0x00000004,
        .length = 60,
        .first = &s3v1,
        .color = {150,255,0, 255},
    },
    {
        .id_int = 0x00000008,
        .length = 60,
        .first = &s3v1,
        .color = {150,255,0, 255},
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

