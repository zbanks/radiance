#ifndef __UI_H
#define __UI_H

#include "core/slot.h"
#include "ui/layout.h"

extern param_state_t * active_param_source;

extern struct xy mouse_drag_start;
extern struct xy mouse_drag_delta;

extern void (*mouse_drag_fn_p)();
extern void (*mouse_drop_fn_p)();

void ui_start(void (*ui_done)());
void ui_stop();

#define HANDLED 1
#define UNHANDLED 0
#define PROPAGATE(x) if(x == HANDLED){return HANDLED;}

SDL_Surface * ui_create_surface_or_die(int width, int height);

#endif
