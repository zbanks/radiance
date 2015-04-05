#ifndef __SLICE_H
#define __SLICE_H

#include <slot.h>

typedef struct output_vertex
{
    float x;
    float y;
    int index;
    struct output_vertex* next;
} output_vertex_t;

typedef struct output_strip
{
    int id;
    int length;
    output_vertex_t* first;
    int bus;
} output_strip_t;

extern int n_output_strips;
extern output_strip_t output_strips[];

void output_to_buffer(output_strip_t* strip, color_t* buffer);

#endif
