#include "slice.h"
#include "slot.h"

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
        .id = 1,
        .length = 200,
        .first = &s1v1,
    },
    {
        .id = 2,
        .length = 200,
        .first = &s2v1,
    },
};

void output_to_buffer(output_strip_t* strip, color_t* buffer)
{
    pthread_mutex_lock(&patterns_updating);

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

    pthread_mutex_unlock(&patterns_updating);
}

