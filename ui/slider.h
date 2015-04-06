#ifndef __SLIDER_H
#define __SLIDER_H

#include <SDL/SDL.h>
#include "core/parameter.h"

extern SDL_Surface* slider_surface;
extern SDL_Surface* output_slider_surface;

void slider_init();
void slider_del();
void slider_render(parameter_t* param, param_state_t* state, SDL_Color c);
void slider_output_render(param_output_t * output);

#endif
