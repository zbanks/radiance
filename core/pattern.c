#include "pattern.h"
#include "slot.h"
#include <stdlib.h>

static color_t param_to_color(float param)
{
    color_t result;
    result.r = param;
    result.g = param;
    result.b = param;
    result.a = 1;
    return result;
}

pat_state_pt pat_full_init()
{
    return malloc(sizeof(color_t));
}

void pat_full_del(pat_state_pt state)
{
    free(state);
}

void pat_full_update(slot_t* slot, float t)
{
    color_t* color = (color_t*)slot->state;
    *color = param_to_color(slot->param_values[0]);
}

color_t pat_full_pixel(slot_t* slot, float x, float y)
{
    return *(color_t*)slot->state;
}

const parameter_t pat_full_params[1] = {
    {
        .name = "Color",
        .default_val = 0.5,
    },
};

const pattern_t pat_full = {
    .render = &pat_full_pixel,
    .init = &pat_full_init,
    .del = &pat_full_del,
    .update = &pat_full_update,
    .n_params = 1,
    .parameters = pat_full_params,
};

/*
pat_state_pt init()
{
    return malloc(sizeof(float));
}

void del(pat_state_pt state)
{
    free((float*)state);
}

void update(slot_t* slot, float t)
{
    float* st = (float*)slot->state;
    *st = t;
}

color_t pixel_at(slot_t* slot, float x, float y)
{
    float* st = (float*)slot->state;
    color_t result;
    float n = 1-sqrt(x*x+y*y);
    if(n < 0) n = 0;

    result.r = 1;
    result.g = slot->param_values[0];
    result.b = 0;
    result.a = n;

    return result;
}

color_t pixel_at2(slot_t* slot, float x, float y)
{
    float* st = (float*)slot->state;

    color_t result;
    float n = 1 - x*x;
    if(n < 0) n = 0;

    result.r = 0;
    result.g = 0;
    result.b = 1;
    result.a = n * fmod(*st, 1);

    return result;
}

const parameter_t ball_parameters[1] = {
    {
        .name = "yellow",
        .default_val = 0.5,
    },
};

const pattern_t ball = {
    .render = &pixel_at,
    .init = &init,
    .del = &del,
    .update = &update,
    .n_params = 1,
    .parameters = ball_parameters,
};

const pattern_t stripe = {
    .render = &pixel_at2,
    .init = &init,
    .del = &del,
    .update = &update,
    .n_params = 0,
};
*/
