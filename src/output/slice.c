#include <SDL/SDL_thread.h>
#include <math.h>

#include "core/config.h"
#include "core/err.h"
#include "core/slot.h"
#include "output/slice.h"

#define C_X1 0.5
#define C_X2 -0.5
#define C_Y 0

output_vertex_t s1v2 = {
    .x = C_X1,
    .y = C_Y,
    .index = 140,
    .next = 0,
};

output_vertex_t s1v1 = {
    .x = 1,
    .y = -1,
    .index = 0,
    .next = &s1v2,
};

output_vertex_t s2v2 = {
    .x = C_X2,
    .y = C_Y,
    .index = 140,
    .next = 0,
};

output_vertex_t s2v1 = {
    .x = -1,
    .y = -1,
    .index = 0,
    .next = &s2v2,
};

output_vertex_t s3v2 = {
    .x = C_X1,
    .y = C_Y,
    .index = 140,
    .next = 0,
};

output_vertex_t s3v1 = {
    .x = C_X2,
    .y = C_Y,
    .index = 0,
    .next = &s3v2,
};

output_vertex_t s4v2 = {
    .x = 1,
    .y = 1,
    .index = 140,
    .next = 0,
};

output_vertex_t s4v1 = {
    .x = C_X1,
    .y = C_Y,
    .index = 0,
    .next = &s4v2,
};

output_vertex_t s5v2 = {
    .x = -1,
    .y = 1,
    .index = 140,
    .next = 0,
};

output_vertex_t s5v1 = {
    .x = C_X2,
    .y = C_Y,
    .index = 0,
    .next = &s5v2,
};


#define N_OUTPUT_STRIPS 5

int n_output_strips = N_OUTPUT_STRIPS;

output_strip_t output_strips[N_OUTPUT_STRIPS] = {
    {
        .id_int = 0x00000001,
        .length = 140,
        .first = &s1v1,
        .color = {255,255,0, 255},
    },
    {
        .id_int = 0x00000002,
        .length = 140,
        .first = &s2v1,
        .color = {255,150,0, 255},
    },
    {
        .id_int = 0x00000003,
        .length = 140,
        .first = &s3v1,
        .color = {150,255,0, 255},
    },
    {
        .id_int = 0x00000004,
        .length = 140,
        .first = &s4v1,
        .color = {150,255,0, 255},
    },
    {
        .id_int = 0x00000005,
        .length = 140,
        .first = &s5v1,
        .color = {150,255,0, 255},
    },
};

void output_to_buffer(output_strip_t* strip, color_t* buffer)
{
    if(SDL_LockMutex(patterns_updating)) FAIL("Unable to lock update mutex: %s\n", SDL_GetError());

    output_vertex_t* vert = strip->first;
    strip->xs = malloc(sizeof(float) * strip->length);
    if(!strip->xs) FAIL("Unable to alloc xs for strip.\n");
    strip->ys = malloc(sizeof(float) * strip->length);
    if(!strip->ys) FAIL("Unable to alloc ys for strip.\n");

    strip->frame = buffer;

    for(int i=0; i<strip->length; i++)
    {
        while(i > vert->next->index)
        {
            vert = vert->next;
            if(!vert->next) goto cleanup; // Error condition
        }
        if(i >= strip->length) break;

        float alpha = (float)(i - vert->index) / (vert->next->index - vert->index);
        strip->xs[i] = alpha * vert->next->x + (1 - alpha) * vert->x;
        strip->ys[i] = alpha * vert->next->y + (1 - alpha) * vert->y;
        //buffer[i] = render_composite(x, y);
    }
    render_composite_frame(STATE_SOURCE_OUTPUT, strip->xs, strip->ys, strip->length, strip->frame);

    for (int i = 0; i < strip->length; i++) {
        strip->frame[i].r = pow(strip->frame[i].r, config.output.gamma);
        strip->frame[i].g = pow(strip->frame[i].g, config.output.gamma);
        strip->frame[i].b = pow(strip->frame[i].b, config.output.gamma);
    }

cleanup:

    free(strip->xs);
    free(strip->ys);

    SDL_UnlockMutex(patterns_updating);
}

