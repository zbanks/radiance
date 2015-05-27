#ifndef __UI_TEXT_H__
#define __UI_TEXT_H__

#include "ui/ui.h"
#include "ui/layout.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

enum txt_alignment {
    TXTALIGN_TL = 0,
    TXTALIGN_BR = 1,
    TXTALIGN_TC = 2,
    TXTALIGN_CC = 3,
    TXTALIGN_TR = 4,
};

void text_load_font(struct txt * params);
void text_render(SDL_Surface * surface, TTF_Font * font, struct txt * params, const SDL_Color * color, const char * text);

#endif
