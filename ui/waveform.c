#include "ui/waveform.h"
#include "ui/layout.h"
#include "core/err.h"
#include "waveform/waveform.h"

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

SDL_Surface* waveform_surface;
int skip = 1;

void ui_waveform_init(){
    waveform_surface = SDL_CreateRGBSurface(0, layout.waveform.w, layout.waveform.h, 32, 0, 0, 0, 0);
    if(!waveform_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    if(layout.waveform.skip * layout.waveform.w < WAVEFORM_HISTORY_SIZE)
        skip = layout.waveform.skip;
}

void ui_waveform_render(){
    rect_t r;
    rect_origin(&layout.waveform.rect, &r);

    SDL_FillRect(waveform_surface, &r, SDL_MapRGB(waveform_surface->format, 20, 20, 20));

    for(int j = 0; j < N_WF_BINS; j++){
        float * history = waveform_bins[j].history;
        SDL_Color c = waveform_bins[j].color;

        for(int i = 0; i < layout.waveform.w; i++)
        {
            float x;
            for(int k = 0; k < skip; k++){
                x += history[i * skip + k];
            }
            x /= (float) skip;
            int h = x * layout.waveform.h;
            vlineRGBA(waveform_surface, i, (layout.waveform.h + h) / 2, (layout.waveform.h - h) / 2, c.r, c.g, c.b, 255);
        }
    }
}

void ui_waveform_del(){

}
