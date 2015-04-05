#ifndef __COLOR_H__
#define __COLOR_H__

typedef struct color
{
    float r;
    float g;
    float b;
    float a;
} color_t;

color_t param_to_color(float param);

#endif
