#include <stdio.h>
#include "err.h"
#include "frame.h"
#include "ui.h"
#include <math.h>

color_t pixel_at(float x, float y, void* state)
{
    color_t result;
    float n = 1-sqrt(x*x+y*y);
    if(n < 0) n = 0;

    result.r = 1;
    result.g = 0;
    result.b = 0;
    result.a = n;

    return result;
}

color_t pixel_at2(float x, float y, void* state)
{
    color_t result;
    float n = 1 - x*x;
    if(n < 0) n = 0;

    result.r = 0;
    result.g = 0;
    result.b = 1;
    result.a = n;

    return result;
}

const pattern_t ball = {
    .render = &pixel_at,
};

const pattern_t stripe = {
    .render = &pixel_at2,
};

int main()
{
    slots[0].pattern = &stripe;
    slots[1].pattern = &ball;

    if(ui_init())
    {
        ERROR("oh no!");
        ui_quit();
        return 1;
    }

    for(;;)
    {
        if(ui_poll()) break;
        ui_render();
    }

    ui_quit();

    return 0;
}

