#include "ui/ui.h"
#include "ui/text.h"
#include "ui/layout.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

void text_render(SDL_Surface * surface, TTF_Font * font, struct txt * params, const SDL_Color * color, const char * text){
    SDL_Surface* msg;
    SDL_Color white = {0,0,0,255};
    rect_t r;

    if(color){
        msg = TTF_RenderText_Solid(font, text, *color);
    }else{
        msg = TTF_RenderText_Solid(font, text, white);
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
