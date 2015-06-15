#include <stdio.h>
#include "ui/text.h"

#include "ui/layout.h"

#include "core/config_gen_c.def"

struct layout layout;

void rect_array_layout(struct rect_array * array_spec, int i, rect_t * rect){
    int index_x = i;
    int index_y = i;
    if(array_spec->tile > 0){
        // Tile down with `tile` per row
        index_x = i % array_spec->tile;
        index_y = i / array_spec->tile;
    }else if(array_spec->tile < 0){
        // Tile across with `tile` per column
        index_y = i % (-array_spec->tile);
        index_x = i / (-array_spec->tile);
    }

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
