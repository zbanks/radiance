#ifndef __UI_H
#define __UI_H
#include "util/common.h"
void ui_init();
void ui_run();
void ui_term();
SDL_GLContext ui_make_secondary_context();
void ui_make_context_current(SDL_GLContext);
#endif
