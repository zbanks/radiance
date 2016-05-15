#ifndef __CORE_CONFIG_COLOR_H__
#define __CORE_CONFIG_COLOR_H__

#include "core/config_macros.h"
#include "SDL/SDL.h"

#define COLOR SDL_Color
#define COLOR_PARSE(x) _parse_color(x)
#define COLOR_FORMAT(x) "#%02hhx%02hhx%02hhx", x.r, x.g, x.b
#define COLOR_FREE(x) (void)(x)
#define COLOR_PREP(x) _parse_color(x)


static inline SDL_Color _parse_color(const char * cstr){
    SDL_Color out;
    if(sscanf(cstr, "#%02hhx%02hhx%02hhx", &out.r, &out.g, &out.b) == 3)
        return out;
    return (SDL_Color) {0, 0, 0, 0};
}

#endif
