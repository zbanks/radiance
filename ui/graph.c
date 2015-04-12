#include "graph.h"
#include <stdlib.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "ui/ui.h"
#include "core/err.h"

SDL_Surface* graph_surface;

void graph_init()
{
    graph_surface = SDL_CreateRGBSurface(0, layout.graph.width, layout.graph.height, 32, 0, 0, 0, 0);
    if(!graph_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
}

void graph_del(graph_state_t* state)
{
    SDL_FreeSurface(graph_surface);
}

void graph_create(graph_state_t* state)
{
    state->history = malloc(sizeof(*(state->history)) * layout.graph.width);
    if(!state->history) FAIL("Could not malloc graph history\n");
    for(int i = 0; i < layout.graph.width; i++)
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
    long t = SDL_GetTicks();
    int pixels = (t - state->last_t) / (1000 / layout.graph.scroll_rate);
    float prev_value = state->history[0];
    for(int i = 1; i <= pixels; i++)
    {
        for(int i = layout.graph.width - 1; i > 0; i--)
        {
            state->history[i] = state->history[i-1];
        }
        float alpha = (float)i / pixels;
        state->history[0] = (1 - alpha) * prev_value + alpha * value;
        state->last_t += (1000 / layout.graph.scroll_rate);
    }
}

static int x_to_pixel(graph_state_t* state, float x)
{
    int pix = (int)round((1 - x) * (layout.graph.height - 1));
    if(pix < 0) pix = 0;
    if(pix >= layout.graph.height) pix = layout.graph.height - 1;
    return pix;
}

void graph_render(graph_state_t* state, SDL_Color c)
{
    SDL_Rect r;

    r.x = 0;
    r.y = 0;
    r.w = layout.graph.width;
    r.h = layout.graph.height;

    SDL_FillRect(graph_surface, &r, SDL_MapRGB(graph_surface->format, 20, 20, 20));

    int prevX = x_to_pixel(state, state->history[0]);
    for(int i = 1; i < layout.graph.width; i++)
    {
        int x = x_to_pixel(state, state->history[i]);
        vlineRGBA(graph_surface, i, prevX, x, c.r, c.g, c.b, 255);
        prevX = x;
    }
}

