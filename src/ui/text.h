#ifndef __UI_TEXT_H__
#define __UI_TEXT_H__

#include "ui/ui.h"
#include "ui/layout.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

void text_load_font(struct txt * params);
void text_unload_fonts();
void text_render(SDL_Surface * surface, struct txt * params, const SDL_Color * color, const char * text);

#endif
