#ifndef __UI_LAYOUT_H__
#define __UI_LAYOUT_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>

#define INT int
#define STRING char *
#define FLOAT float

#define INT_FN(x) atoi(x)
#define STRING_FN(x) strdup(x)
#define FLOAT_FN(x) atof(x)

#define INT_FMT "%d"
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
    CFG(PREFIX(n, x), INT, dx) CFG(PREFIX(n, y), INT, dy)
struct PACKED xy {
    CFG_XY_ATTR(,,)
};

#define CFG_TXT_ATTR(n, dx, dy, dsize, dalign, dfont, dcolor) \
    CFG_XY_ATTR(n, dx, dy) \
    CFG(PREFIX(n, size), INT, dsize) \
    CFG(PREFIX(n, align), INT, dalign) \
    CFG(PREFIX(n, font), STRING, dfont) \
    CFG(PREFIX(n, color), STRING, dcolor)

union txt { 
    struct xy xy;
    struct PACKED {
        CFG_TXT_ATTR(,,,,,,)
    };
};

#define CFG_RECT_ATTR(n, dx, dy, dw, dh) \
    CFG(PREFIX(n, x), INT, dx) \
    CFG(PREFIX(n, y), INT, dy) \
    CFG(PREFIX(n, w), INT, dw) \
    CFG(PREFIX(n, h), INT, dh) 

union rect {
    SDL_Rect sdl;
    struct PACKED {
        struct xy xy;
        struct xy wh;
    };
    struct PACKED {
        CFG_RECT_ATTR(,,,,)
    };
};

#define CFG_RECT_ARRAY_ATTR(n, dx, dy, dw, dh, dpx, dpy, dtile) \
    CFG(PREFIX(n, x), INT, dx) \
    CFG(PREFIX(n, y), INT, dy) \
    CFG(PREFIX(n, w), INT, dw) \
    CFG(PREFIX(n, h), INT, dh) \
    CFG(PREFIX(n, px), INT, dpx) \
    CFG(PREFIX(n, py), INT, dpy) \
    CFG(PREFIX(n, tile), INT, dtile)

struct PACKED rect_array {
    CFG_RECT_ARRAY_ATTR(,,,,,,,)
};

#undef CFG

#define CFG_XY(n, args...) union { struct xy PREFIX(n, xy); struct PACKED { CFG_XY_ATTR(n, args) }; };
#define CFG_TXT(n, args...) union { union txt PREFIX(n, txt); struct PACKED { CFG_TXT_ATTR(n, args) }; };
#define CFG_RECT(n, args...) union { union rect PREFIX(n, rect); struct PACKED { CFG_RECT_ATTR(n, args) }; };
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
void rect_array_layout(struct rect_array * array_spec, int index, union rect * rect);

inline void rect_shift(union rect * rect, struct xy * d){
    rect->x += d->x;
    rect->y += d->y;
}

inline void rect_shift_origin(union rect * rect){
    rect->x = 0;
    rect->y = 0;
}

inline void rect_copy(union rect * dst, union rect * src){
    memcpy(dst, src, sizeof(union rect));
}

inline int in_rect(struct xy * xy, union rect * rect, struct xy * offset){
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


#endif
