#ifndef __UI_LAYOUT_H__
#define __UI_LAYOUT_H__

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "core/config_macros.h"
#include "core/config_color.h"
#include "core/config.h"
#include "core/err.h"
#include "ui/layout_constants.h"
#include "util/string.h"
#include "util/color.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif
#define CFGOBJ layout
#define CFGOBJ_PATH "ui/layout.def"

#define CFG(n, type, def) type n;

struct bmp {
    char * filename;
    SDL_Surface * image;
};

static inline struct bmp _parse_bmp(const char * str){
    struct bmp bmp = {0, 0};
    bmp.filename = strdup(str);

    char * full_path = strcatdup(config.path.images, str);
    if(full_path){
        bmp.image = SDL_LoadBMP(full_path);
        if(!bmp.image){
            ERROR("SDL_LoadBMP: %s\n", SDL_GetError());
        }else{
            // Set the color key to #FEFE00
            //SDL_SetColorKey(bmp.image, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(bmp.image->format, 254, 254, 0));
        }
    }
    return bmp;
}

#define BMP struct bmp
#define BMP_PARSE(x) _parse_bmp(x)
#define BMP_FORMAT(x) "%s", x.filename
#define BMP_FREE(x) free(x.filename); SDL_FreeSurface(x.image);
#define BMP_PREP(x) _parse_bmp(x)

#define CFG_XY_ATTR(n, dx, dy) \
    CFG(PREFIX(n, x), SINT16, dx) CFG(PREFIX(n, y), SINT16, dy)
struct xy {
    CFG_XY_ATTR(,,)
};

#define CFG_TXT_ATTR(n, dx, dy, dsize, dalign, dfont, dcolor) \
    CFG_XY_ATTR(n, dx, dy) \
    CFG(PREFIX(n, size), INT, dsize) \
    CFG(PREFIX(n, align), INT, dalign) \
    CFG(PREFIX(n, font), STRING, dfont) \
    CFG(PREFIX(n, color), COLOR, dcolor)

struct txt;
struct txt {
    CFG_TXT_ATTR(,,,,,,)
    struct {
        TTF_Font * font;
        struct txt * next;
    } ui_font;
};

#define CFG_RECT_ATTR(n, dx, dy, dw, dh) \
    CFG(PREFIX(n, x), SINT16, dx) \
    CFG(PREFIX(n, y), SINT16, dy) \
    CFG(PREFIX(n, w), UINT16, dw) \
    CFG(PREFIX(n, h), UINT16, dh) 


typedef SDL_Rect rect_t;
/*
typedef union {
    SDL_Rect sdl;
    struct {
        struct xy xy;
        struct xy wh;
    };
    struct {
        CFG_RECT_ATTR(,,,,)
    };
} rect_t;
*/

#define CFG_RECT_ARRAY_ATTR(n, dx, dy, dw, dh, dpx, dpy, dtile) \
    CFG(PREFIX(n, x), SINT16, dx) \
    CFG(PREFIX(n, y), SINT16, dy) \
    CFG(PREFIX(n, w), UINT16, dw) \
    CFG(PREFIX(n, h), UINT16, dh) \
    CFG(PREFIX(n, px), UINT16, dpx) \
    CFG(PREFIX(n, py), UINT16, dpy) \
    CFG(PREFIX(n, tile), INT, dtile)

struct rect_array {
    CFG_RECT_ARRAY_ATTR(,,,,,,,)
};

#define CFG_BACKGROUND_ATTR(n, dcolor, dfilename) \
    CFG(PREFIX(n, color), COLOR, dcolor) \
    CFG(PREFIX(n, bmp), BMP, dfilename)

struct background {
    CFG_BACKGROUND_ATTR(,,)
};

#undef CFG

#define CFG_XY(n, args...) union { struct xy PREFIX(n, xy); struct { CFG_XY_ATTR(n, args) }; };
#define CFG_TXT(n, args...) union { struct txt PREFIX(n, txt); struct { CFG_TXT_ATTR(n, args) }; };
#define CFG_RECT(n, args...) union { rect_t PREFIX(n, rect); struct { CFG_RECT_ATTR(n, args) }; };
#define CFG_RECT_ARRAY(n, args...) union { struct rect_array PREFIX(n, rect_array); struct { CFG_RECT_ARRAY_ATTR(n, args) }; };
#define CFG_BACKGROUND(n, args...) union { struct background PREFIX(n, background); struct { CFG_BACKGROUND_ATTR(n, args) }; };

#include "core/config_gen_h.def"

#undef CFG_XY
#define CFG_XY CFG_XY_ATTR

#undef CFG_TXT
#define CFG_TXT CFG_TXT_ATTR

#undef CFG_RECT
#define CFG_RECT CFG_RECT_ATTR

#undef CFG_RECT_ARRAY
#define CFG_RECT_ARRAY CFG_RECT_ARRAY_ATTR

#undef CFG_BACKGROUND
#define CFG_BACKGROUND CFG_BACKGROUND_ATTR

// Calculates the `rect` corresponding to the `index`th item in the `array_spec` layout
void rect_array_layout(struct rect_array * array_spec, int index, rect_t * rect);
// Calculates the `rect` at (0,0) with the width, height of a rect in the `array_spec`
void rect_array_origin(struct rect_array * array_spec, rect_t * rect);

// Offsets a `rect` by an xy coordinate `d`
static inline void rect_shift(rect_t * rect, const struct xy * d){
    rect->x += d->x;
    rect->y += d->y;
}

// Creates a copy of a `rect` at the origin 
static inline void rect_origin(const rect_t * src, rect_t * dst){
    dst->x = 0;
    dst->y = 0;
    dst->w = src->w;
    dst->h = src->h;
}

// Creates a copy of a `rect`
static inline void rect_copy(rect_t * dst, const rect_t * src){
    memcpy(dst, src, sizeof(rect_t));
}

// Determines if an `xy` coordinate is in a given `rect` and where (`offset`) in the rect it is.
// Returns 1 if the coordinate is in the rect & populates `offset` if non-null
static inline int xy_in_rect(const struct xy * xy, const rect_t * rect, struct xy * offset){
    if((xy->x >= rect->x) && (xy->x < rect->x + rect->w) && \
       (xy->y >= rect->y) && (xy->y < rect->y + rect->h)){
        if(offset){
            offset->x = xy->x - rect->x;
            offset->y = xy->y - rect->y;
        }
        return 1;
    }
    return 0;
}

// Adds two `xy` values & returns the result
static inline struct xy xy_add(struct xy a, struct xy b){
    struct xy result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

// Subtracts two `xy` values (a - b) & returns the result
static inline struct xy xy_sub(struct xy a, struct xy b){
    struct xy result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

// Draws a background image onto a SDL_Surface, defaulting to a solid color if an image isn't found
static inline void fill_background(SDL_Surface * surface, rect_t * rect, struct background * bg){
    if(bg->bmp.image){
        // width/height are taken from srcrect, x/y are taken from dstrect
        rect_t rwh = {.x = 0, .y = 0, .w = rect->w, .h = rect->h};
        SDL_BlitSurface(bg->bmp.image, &rwh, surface, rect);
    }else{
        SDL_FillRect(surface, rect, map_sdl_color(surface, bg->color));
    }
}

extern struct layout layout;

#endif
