#ifndef __UI_H
#define __UI_H

#include "core/slot.h"
#include "ui/layout.h"

extern param_state_t * active_param_source;

void ui_init();
void ui_quit();
void ui_update_master();
void ui_render();
int ui_poll();

#endif
