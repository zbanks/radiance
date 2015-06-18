#include <string.h>
#include "core/err.h"
#include "ui/ui.h"
#include "ui/text.h"
#include "ui/layout.h"
#include "ui/layout_constants.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

static struct txt * last_txt = 0;

void text_load_font(struct txt * params){
    char * filename = strcatdup(config.path.fonts, params->font);
    if(!filename) return;

    params->ui_font.font = TTF_OpenFont(filename, params->size);
    if(!params->ui_font.font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    free(filename);

    params->ui_font.next = last_txt;
    last_txt = params;
}

void text_unload_fonts(){
    while (last_txt){
        TTF_CloseFont(last_txt->ui_font.font);
        last_txt = last_txt->ui_font.next;
    }
}

void text_render(SDL_Surface * surface, struct txt * params, const SDL_Color * color, const char * text){
    SDL_Surface* msg;
    rect_t r;

    if(!params->ui_font.font)
        text_load_font(params);

    if(!color) 
        color = &params->color;

    msg = TTF_RenderText_Solid(params->ui_font.font, text, *color);
    if(!msg) return;

    r.x = params->x;
    r.y = params->y;
    r.w = msg->w;
    r.h = msg->h;
    
    switch(params->align){
        case LAYOUT_ALIGN_BR:
            r.y -= r.h; // fall through
        case LAYOUT_ALIGN_TR:
            r.x -= r.w;
            break;
        case LAYOUT_ALIGN_CC:
            r.y -= r.h / 2; // fall through
        case LAYOUT_ALIGN_TC:
            r.x -= r.w / 2;
            break;
        case LAYOUT_ALIGN_TL:
        default:
            break;
    }

    SDL_BlitSurface(msg, 0, surface, &r);
    SDL_FreeSurface(msg);
}
