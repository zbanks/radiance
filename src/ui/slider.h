#ifndef __SLIDER_H
#define __SLIDER_H

#include <SDL/SDL.h>
#include "core/parameter.h"
#include "ui/layout.h"

extern SDL_Surface* slider_surface;
extern SDL_Surface* alpha_slider_surface;
extern SDL_Surface* output_slider_surface;

enum slider_event {
    SLIDER_LOCKED = 1,
    SLIDER_DRAG_START,
    SLIDER_DRAG_END,
    SLIDER_CONNECT_START,
    SLIDER_CONNECT_CANCEL,
    SLIDER_CONNECT,
};

void slider_init();
void slider_del();
void slider_render(const parameter_t* param, param_state_t* state, SDL_Color c);
void slider_render_alpha(param_state_t* state);
void slider_output_render(param_output_t * output);

void mouse_drag_alpha_slider();
void mouse_drag_param_slider();
int mouse_down_alpha_slider(param_state_t * param_state, struct xy xy);
int mouse_down_param_slider(param_state_t * param_state, struct xy xy);

#endif
