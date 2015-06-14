#ifndef __UI_LAYOUT_H__
#define __UI_LAYOUT_H__

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "core/config_macros.h"
#include "ui/layout_constants.h"

#define COLOR SDL_Color
#define COLOR_FN(x) parse_color(x)
#define COLOR_FMT(x) "#%02hhx%02hhx%02hhx", x.r, x.g, x.b
// This is the most hacky... XXX :(
#define _COLOR(r,g,b) ((r << 16) | (g << 8) | b)

static inline SDL_Color parse_color(const char * cstr){
    SDL_Color out;
    if(sscanf(cstr, "#%02hhx%02hhx%02hhx", &out.r, &out.g, &out.b) >= 3)
        return out;
    return (SDL_Color) {0, 0, 0, 0};
}

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ layout
#define CFGOBJ_PATH "ui/layout.def"

//#undef PACKED
//#define PACKED 

#define CFG(n, type, def) type n;

#define CFG_XY_ATTR(n, dx, dy) \
    CFG(PREFIX(n, x), SINT16, dx) CFG(PREFIX(n, y), SINT16, dy)
struct PACKED xy {
    CFG_XY_ATTR(,,)
};

#define CFG_TXT_ATTR(n, dx, dy, dsize, dalign, dfont, dcolor) \
    CFG_XY_ATTR(n, dx, dy) \
    CFG(PREFIX(n, size), INT, dsize) \
    CFG(PREFIX(n, align), INT, dalign) \
    CFG(PREFIX(n, font), STRING, dfont) \
    CFG(PREFIX(n, color), STRING, dcolor)

struct txt;
struct PACKED txt {
    CFG_TXT_ATTR(,,,,,,)
    struct {
        TTF_Font * font;
        SDL_Color color;
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
    struct PACKED {
        struct xy xy;
        struct xy wh;
    };
    struct PACKED {
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

struct PACKED rect_array {
    CFG_RECT_ARRAY_ATTR(,,,,,,,)
};

#undef CFG

#define CFG_XY(n, args...) union { struct xy PREFIX(n, xy); struct PACKED { CFG_XY_ATTR(n, args) }; };
#define CFG_TXT(n, args...) union { struct txt PREFIX(n, txt); struct PACKED { CFG_TXT_ATTR(n, args) }; };
#define CFG_RECT(n, args...) union { rect_t PREFIX(n, rect); struct PACKED { CFG_RECT_ATTR(n, args) }; };
#define CFG_RECT_ARRAY(n, args...) union { struct rect_array PREFIX(n, rect_array); struct PACKED { CFG_RECT_ARRAY_ATTR(n, args) }; };

#include "core/config_gen_h.def"

#undef CFG_XY
#define CFG_XY CFG_XY_ATTR

#undef CFG_TXT
#define CFG_TXT CFG_TXT_ATTR

#undef CFG_RECT
#define CFG_RECT CFG_RECT_ATTR

#undef CFG_RECT_ARRAY
#define CFG_RECT_ARRAY CFG_RECT_ARRAY_ATTR

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
    return (struct xy) {
        .x = a.x + b.x,
        .y = a.y + b.y
    };
}

// Subtracts two `xy` values (a - b) & returns the result
static inline struct xy xy_sub(struct xy a, struct xy b){
    return (struct xy) {
        .x = a.x - b.x,
        .y = a.y - b.y
    };
}

#endif
