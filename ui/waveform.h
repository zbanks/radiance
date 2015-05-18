#ifndef __UI_WAVEFORM_H__
#define __UI_WAVEFORM_H__

#include "waveform/waveform.h"
#include <SDL/SDL.h>

extern SDL_Surface* waveform_surface;

void ui_waveform_render();
void ui_waveform_init();
void ui_waveform_del();

#endif
