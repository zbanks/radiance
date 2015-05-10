#ifndef __UI_LAYOUT_H__
#define __UI_LAYOUT_H__

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>

char* mystrdup(const char* s);

#define INT int
#define SINT16 Sint16
#define UINT16 Uint16
#define STRING char *
#define FLOAT float

#define INT_FN(x) atoi(x)
#define SINT16_FN(x) atoi(x)
#define UINT16_FN(x) atoi(x)
#define STRING_FN(x) mystrdup(x)
#define FLOAT_FN(x) atof(x)

#define INT_FMT "%d"
#define SINT16_FMT "%d"
#define UINT16_FMT "%d"
#define STRING_FMT "%s"
#define FLOAT_FM "%f"

#define PACKED __attribute__ ((__packed__))

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x ## y

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

/* ISEMPTY macro from https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/ */

#define _ARG16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define HAS_COMMA(...) _ARG16(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define _TRIGGER_PARENTHESIS_(...) ,
 
#define ISEMPTY(...)                                                    \
_ISEMPTY(                                                               \
          /* test if there is just one argument, eventually an empty    \
             one */                                                     \
          HAS_COMMA(__VA_ARGS__),                                       \
          /* test if _TRIGGER_PARENTHESIS_ together with the argument   \
             adds a comma */                                            \
          HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__),                 \
          /* test if the argument together with a parenthesis           \
             adds a comma */                                            \
          HAS_COMMA(__VA_ARGS__ (/*empty*/)),                           \
          /* test if placing it between _TRIGGER_PARENTHESIS_ and the   \
             parenthesis adds a comma */                                \
          HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/))      \
          )
 
#define PASTE5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define _ISEMPTY(_0, _1, _2, _3) HAS_COMMA(PASTE5(_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define _IS_EMPTY_CASE_0001 ,

/* End of ISEMPTY macro code */

#define PREFIX(prefix, suffix) CONCAT(CONCAT(PREFIX_, ISEMPTY(prefix))(prefix), suffix)
#define PREFIX_1(prefix) prefix
#define PREFIX_0(prefix) CONCAT(prefix, _)

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

struct PACKED txt {
    CFG_TXT_ATTR(,,,,,,)
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

struct layout {
    #define CFGSECTION(w, d...) struct PACKED { d } w;
    #define CFG(n, type, default) type n;
    #include "layout.def"
};

extern struct layout layout;

#undef CFG_XY
#define CFG_XY CFG_XY_ATTR

#undef CFG_TXT
#define CFG_TXT CFG_TXT_ATTR

#undef CFG_RECT
#define CFG_RECT CFG_RECT_ATTR

#undef CFG_RECT_ARRAY
#define CFG_RECT_ARRAY CFG_RECT_ARRAY_ATTR

int dump_layout(struct layout* cfg, char * filename);
void rect_array_layout(struct rect_array * array_spec, int index, rect_t * rect);
void rect_array_origin(struct rect_array * array_spec, rect_t * rect);

static inline void rect_shift(rect_t * rect, const struct xy * d){
    rect->x += d->x;
    rect->y += d->y;
}

static inline void rect_shift_origin(rect_t * rect){
    rect->x = 0;
    rect->y = 0;
}

static inline void rect_copy(rect_t * dst, const rect_t * src){
    memcpy(dst, src, sizeof(rect_t));
}

static inline int in_rect(const struct xy * xy, const rect_t * rect, struct xy * offset){
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

static inline struct xy xy_add(struct xy a, struct xy b){
    return (struct xy) {
        .x = a.x + b.x,
        .y = a.y + b.y
    };
}

static inline struct xy xy_sub(struct xy a, struct xy b){
    return (struct xy) {
        .x = a.x - b.x,
        .y = a.y - b.y
    };
}

#endif
