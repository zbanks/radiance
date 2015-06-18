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

// BMP images are lazily loaded. Try to load one 
SDL_Surface * bmp_load(struct bmp * bmp){
    if(bmp->error) return NULL; 
    if(!bmp->image){
        char * full_path = strcatdup(config.path.images, bmp->filename);
        if(full_path) {
            bmp->image = SDL_LoadBMP(full_path);
            if(!bmp->image) {
                ERROR("SDL_LoadBMP: %s\n", SDL_GetError());
                bmp->error = 1;
            } else {
                SDL_DisplayFormat(bmp->image);
                // Set the color key to #FE00FE
                SDL_SetColorKey(bmp->image, SDL_SRCCOLORKEY, SDL_MapRGB(bmp->image->format, 254, 0, 254));
            }
        }
        free(full_path);
    }
    return bmp->image;
}

// Draws a background image onto a SDL_Surface, defaulting to a solid color if an image isn't found
void fill_background(SDL_Surface * surface, rect_t * rect, struct background * bg){
    SDL_Surface * image = bmp_load(&bg->bmp);
    if(image){
        // width/height are taken from srcrect, x/y are taken from dstrect
        rect_t rwh = {.x = 0, .y = 0, .w = rect->w, .h = rect->h};
        SDL_BlitSurface(image, &rwh, surface, rect);
    }else{
        SDL_FillRect(surface, rect, map_sdl_color(surface, bg->color));
    }
}
