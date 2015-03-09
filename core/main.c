#include <stdio.h>
#include "err.h"
#include "frame.h"
#include "ui.h"
#include <math.h>
#include <stdlib.h>

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
    result.g = 0;
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

int main()
{
    if(ui_init())
    {
        ERROR("oh no!");
        ui_quit();
        return 1;
    }

    slots[0].pattern = &stripe;
    slots[0].state = (*slots[0].pattern->init)();

    slots[1].pattern = &ball;
    slots[1].state = (*slots[1].pattern->init)();

    for(int i = 0; i < n_slots; i++)
    {
        ui_update_slot(&slots[i]);
    }

    for(;;)
    {
        float t = (float)SDL_GetTicks() / 1000.;

        if(ui_poll()) break;
        update_patterns(t);
        ui_render();
        // TODO rate-limit
    }

    (*slots[0].pattern->del)(slots[0].state);
    (*slots[1].pattern->del)(slots[1].state);
    ui_quit();

    return 0;
}

