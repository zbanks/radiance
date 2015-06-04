#include <stdio.h>
#include "ui/text.h"

#include "ui/layout.h"

#include "core/config_gen_c.def"

void rect_array_layout(struct rect_array * array_spec, int index, rect_t * rect){
    if(array_spec->tile != 0){
        printf("Unsupported tile format: %d\n", array_spec->tile);
    }
    int index_x = index;
    int index_y = index;

    rect->x = array_spec->x + array_spec->px * index_x;
    rect->y = array_spec->y + array_spec->py * index_y;
    rect->w = array_spec->w;
    rect->h = array_spec->h;
}

void rect_array_origin(struct rect_array * array_spec, rect_t * rect){
    rect->x = 0;
    rect->y = 0;
    rect->w = array_spec->w;
    rect->h = array_spec->h;
}
