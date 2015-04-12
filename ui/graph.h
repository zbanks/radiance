#ifndef __GRAPH_H
#define __GRAPH_H

#include <SDL/SDL.h>

typedef struct
{
    float* history;
    long last_t;
    int width;
    int height;
    int scroll_rate;
} graph_state_t;

extern SDL_Surface* graph_surface;

void graph_init();
void graph_del();
void graph_create(graph_state_t* state, int width);
void graph_remove(graph_state_t* state);

void graph_update(graph_state_t* state, float value);
void graph_render(graph_state_t* state, SDL_Color c);

#endif
