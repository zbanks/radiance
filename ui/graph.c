#include "graph.h"
#include <stdlib.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "ui/ui.h"
#include "core/err.h"
#include "waveform/waveform.h"

SDL_Surface* graph_surface;

#define MAX(x,y) ((x > y) ? x : y)

void graph_init()
{
    graph_surface = SDL_CreateRGBSurface(0, 
                                         MAX(layout.graph_filter.w, layout.graph_signal.w),
                                         MAX(layout.graph_filter.h, layout.graph_signal.h),
                                         32, 0, 0, 0, 0);
    if(!graph_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
}

void graph_del()
{
    SDL_FreeSurface(graph_surface);
}

void graph_create_filter(graph_state_t* state)
{
    state->width = layout.graph_filter.w;
    state->height = layout.graph_filter.h;
    state->scroll_rate = layout.graph_filter.scroll_rate;
    state->history = malloc(sizeof(*(state->history)) * state->width);
    if(!state->history) FAIL("Could not malloc graph history\n");
    for(int i = 0; i < state->width; i++)
    {
        state->history[i] = 0;
    }
    state->last_t = SDL_GetTicks();
}

void graph_create_signal(graph_state_t* state)
{
    state->width = layout.graph_signal.w;
    state->height = layout.graph_signal.h;
    state->scroll_rate = layout.graph_signal.scroll_rate;
    state->history = malloc(sizeof(*(state->history)) * state->width);
    if(!state->history) FAIL("Could not malloc graph history\n");
    for(int i = 0; i < state->width; i++)
    {
        state->history[i] = 0;
    }
    state->last_t = SDL_GetTicks();
}

void graph_remove(graph_state_t* state)
{
    free(state->history);
}

void graph_update(graph_state_t* state, float value)
{
    if(!state->history) return;
    long t = SDL_GetTicks();
    int pixels = (t - state->last_t) / (1000 / state->scroll_rate);
    if(pixels <= 0) return; 
    if(pixels >= state->width) pixels = state->width - 1;

    float prev_value = state->history[0];
    // TODO XXX There's an issue where memory is being overwritten
    if(isnan(prev_value)) prev_value = 0.;
    memmove(state->history+pixels, state->history, (state->width - pixels) * sizeof(float));
    for(int i = 0; i < pixels; i++)
    {
        float alpha = ((float) i) / ((float) pixels);
        state->history[i] = (1 - alpha) * value + alpha * prev_value;
        //XXX Band-aid: something is overwriting history[0] w/ -nan
        //if(isnan(state->history[i])) state->history[i] = 0.;
    }
    state->last_t += pixels * (1000 / state->scroll_rate);
}

static int x_to_pixel(graph_state_t* state, float x)
{
    int pix = (int)round((1 - x) * (state->height - 1));
    if(pix < 0) pix = 0;
    if(pix >= state->height) pix = state->height - 1;
    return pix;
}

void graph_render(graph_state_t* state, SDL_Color c)
{
    rect_t r;
    if(!state->history) return;

    r.x = 0;
    r.y = 0;
    r.w = graph_surface->w;
    r.h = graph_surface->h;

    SDL_FillRect(graph_surface, &r, SDL_MapRGB(graph_surface->format, 20, 20, 20));

    r.w = state->width;
    r.h = state->height;

    SDL_FillRect(graph_surface, &r, SDL_MapRGB(graph_surface->format, 30, 30, 30));

    int prevX = x_to_pixel(state, state->history[0]);
    for(int i = 1; i < state->width; i++)
    {
        int x = x_to_pixel(state, state->history[i]);
        int delta = 0;
        if(x > prevX) delta = 1;
        if(x < prevX) delta = -1;
        vlineRGBA(graph_surface, state->width - i, prevX + delta, x, c.r, c.g, c.b, 255);
        prevX = x;
    }
}

