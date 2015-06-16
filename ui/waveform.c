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

    SDL_FillRect(waveform_surface, &r, SDL_MapRGB(waveform_surface->format, 30, 30, 30));
#define MAX(a, b) ((a > b) ? a : b)
    int h;
    SDL_Color c;
    if(skip < 1) skip = 1;

    for(int j = 0; j < N_WF_BINS; j++){
        float * history = waveform_bins[j].history;
        int hptr = waveform_bins[j].hptr; 
        hptr -= hptr % skip;
        c = waveform_bins[j].color;

        for(int i = 0; i < layout.waveform.w; i++)
        {
            float x = 0.;
            for(int k = 0; k < skip; k++){
                hptr--;
                if(hptr < 0)
                    hptr = WAVEFORM_HISTORY_SIZE;
                x = MAX(x, history[hptr]);
            }
            h = x * layout.waveform.h;
            vlineRGBA(waveform_surface, layout.waveform.w - i, (layout.waveform.h + h) / 2, (layout.waveform.h - h) / 2, c.r, c.g, c.b, 255);
        }
    }
    /*
    c = beat_bin.color;
    int last_h = 0;
    for(int i = 0; i < layout.waveform.w; i++)
    {
        h = beat_bin.history[i * skip + skip / 2] * layout.waveform.h;
        vlineRGBA(waveform_surface, i, h, last_h-1, c.r, c.g, c.b, 255);
        last_h = h;
    }
    */
    for(int i = 0; i < layout.waveform.w; i++){
        char l = 0;
        for(int k = 0; k < skip; k++){
            l = l | beat_lines[i * skip + k];
        }
        if(l & 1)
            vlineRGBA(waveform_surface, layout.waveform.w - i, layout.waveform.y, layout.waveform.h, 128, 255, 0, 200);
        if(l & 2)
            vlineRGBA(waveform_surface, layout.waveform.w - i, layout.waveform.y, layout.waveform.h, 255, 128, 0, 200);
        if(l & 4)
            vlineRGBA(waveform_surface, layout.waveform.w - i, layout.waveform.y, layout.waveform.h, 255, 0, 0, 200);
         
    }
}

void ui_waveform_del(){

}
