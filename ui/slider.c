#include "ui/slider.h"
#include "core/err.h"
#include "ui/ui.h"

#include <SDL/SDL_ttf.h>

SDL_Surface* slider_surface;

static TTF_Font* param_font;

void slider_init()
{
    slider_surface = SDL_CreateRGBSurface(0, layout.slider.w, layout.slider.h, 32, 0, 0, 0, 0);
    if(!slider_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    param_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 10);
    if(!param_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());
}

void slider_del()
{
    SDL_FreeSurface(slider_surface);
    TTF_CloseFont(param_font);
}

void slider_render(parameter_t* param, param_state_t* state, SDL_Color c)
{
    param_output_t * param_output = param_state_output(state);
    SDL_Color handle_color = {0, 0, 80};
    SDL_Color white = {255, 255, 255};
    SDL_Surface* txt = TTF_RenderText_Solid(param_font, param->name, c);

    SDL_Rect r;
    SDL_FillRect(slider_surface, &layout.slider.rect, SDL_MapRGB(slider_surface->format, 20, 20, 20));

    r.x = layout.slider.name_x;
    r.y = layout.slider.name_y;
    r.w = txt->w;
    r.h = txt->h;
    SDL_BlitSurface(txt, 0, slider_surface, &r);
    SDL_FreeSurface(txt);

    if(param->val_to_str){
        char sbuf[129];
        param->val_to_str(param_state_get(state), sbuf, 128);
        txt = TTF_RenderText_Solid(param_font, sbuf, white);
        r.x = layout.slider.value_x;
        r.y = layout.slider.value_y;
        r.w = txt->w;
        r.h = txt->h;
        SDL_BlitSurface(txt, 0, slider_surface, &r);
        SDL_FreeSurface(txt);
    }

    if(param_output){
        handle_color = param_output->handle_color;

        txt = TTF_RenderText_Solid(param_font, param_output->label, param_output->label_color);
        r.x = layout.slider.source_x - txt->w;
        r.y = layout.slider.source_y;
        r.w = txt->w;
        r.h = txt->h;
        SDL_BlitSurface(txt, 0, slider_surface, &r);
        SDL_FreeSurface(txt);
    }

    r.x = layout.slider.track_x;
    r.y = layout.slider.track_y;
    r.w = layout.slider.track_w;
    r.h = 3;
    SDL_FillRect(slider_surface, &r, SDL_MapRGB(slider_surface->format, 80, 80, 80));

    r.x = layout.slider.handle_x +
          param_state_get(state) * (layout.slider.track_w - layout.slider.handle_w);
    r.y = layout.slider.handle_y;
    r.w = layout.slider.handle_w;
    r.h = layout.slider.handle_h;


    SDL_FillRect(slider_surface, &r, SDL_MapRGB(slider_surface->format,
                                                handle_color.r,
                                                handle_color.g,
                                                handle_color.b));
}
