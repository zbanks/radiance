#ifndef __COLOR_H__
#define __COLOR_H__

#include <SDL/SDL.h>

typedef struct color
{
    float r;
    float g;
    float b;
    float a;
} color_t;

uint32_t color_to_MapRGB(const SDL_PixelFormat * format, color_t color);
SDL_Color color_to_SDL(color_t color);
color_t param_to_color(float param);

#endif
