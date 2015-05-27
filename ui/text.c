#include <string.h>
#include "core/err.h"
#include "ui/ui.h"
#include "ui/text.h"
#include "ui/layout.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

void text_load_font(struct txt * params){
    SDL_Color color = {255,255,255,255};
    char filename[256];

    strncpy(filename, layout.globals.font_directory, 255);
    //TODO XXX: no bounds checking
    strcat(filename, params->font);

    params->ui_font.font = TTF_OpenFont(filename, params->size);
    if(!params->ui_font.font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    unsigned int r, g, b;
    if(sscanf(params->color, "#%2x%2x%2x", &r, &g, &b) == 3){
        color.r = r;
        color.g = g;
        color.b = b;
    }else{
        printf("Unable to parse color '%s'; using white instead.", params->color);
    }
    params->ui_font.color = color;
}

void text_render(SDL_Surface * surface, TTF_Font * font, struct txt * params, const SDL_Color * color, const char * text){
    SDL_Surface* msg;
    rect_t r;
    UNUSED(font);

    if(!params->ui_font.font)
        text_load_font(params);

    if(color){
        msg = TTF_RenderText_Solid(params->ui_font.font, text, *color);
    }else{
        msg = TTF_RenderText_Solid(params->ui_font.font, text, params->ui_font.color);
    }
    if(!msg)
        return;

    r.x = params->x;
    r.y = params->y;
    r.w = msg->w;
    r.h = msg->h;
    //TODO: font, color
    
    // Extract font/color from params
    switch(params->align){
        case TXTALIGN_BR:
            r.y -= r.h;
        case TXTALIGN_TR:
            r.x -= r.w;
            break;
        case TXTALIGN_CC:
            r.y -= r.h / 2; // fall through
        case TXTALIGN_TC:
            r.x -= r.w / 2;
            break;
        case TXTALIGN_TL:
        default:
            break;
    }

    SDL_BlitSurface(msg, 0, surface, &r);
    SDL_FreeSurface(msg);
}
