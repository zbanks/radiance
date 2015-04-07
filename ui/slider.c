#include "ui/slider.h"
#include "core/err.h"
#include "ui/ui.h"

#include <SDL/SDL_ttf.h>

SDL_Surface* slider_surface;
SDL_Surface* output_slider_surface;

static TTF_Font* param_font;

void slider_init()
{
    slider_surface = SDL_CreateRGBSurface(0, layout.slider.width, layout.slider.height, 32, 0, 0, 0, 0);
    if(!slider_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    output_slider_surface = SDL_CreateRGBSurface(0, layout.output_slider.width, layout.output_slider.height, 32, 0, 0, 0, 0);
    if(!slider_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    param_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 10);
    if(!param_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());
}

void slider_del()
{
    SDL_FreeSurface(slider_surface);
    SDL_FreeSurface(output_slider_surface);
    TTF_CloseFont(param_font);
}

void slider_render(parameter_t* param, param_state_t* state, SDL_Color c)
{
    param_output_t * param_output = param_state_output(state);
    SDL_Color handle_color = {0, 0, 80};
    SDL_Color white = {255, 255, 255};
    SDL_Surface* txt = TTF_RenderText_Solid(param_font, param->name, c);

    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.slider.width;
    r.h = layout.slider.height;
    SDL_FillRect(slider_surface, &r, SDL_MapRGB(slider_surface->format, 20, 20, 20));

    r.x = layout.slider.name_x;
    r.y = layout.slider.name_y;
    r.w = txt->w;
    r.h = txt->h;
    SDL_BlitSurface(txt, 0, slider_surface, &r);

    if(param->val_to_str){
        char sbuf[129];
        param->val_to_str(param_state_get(state), sbuf, 128);
        txt = TTF_RenderText_Solid(param_font, sbuf, white);
        r.x = layout.slider.value_start_x;
        r.y = layout.slider.value_start_y;
        r.w = txt->w;
        r.h = txt->h;
        SDL_BlitSurface(txt, 0, slider_surface, &r);
    }

    SDL_FreeSurface(txt);

    if(param_output){
        handle_color = param_output->handle_color;

        txt = TTF_RenderText_Solid(param_font, param_output->label, param_output->label_color);
        r.x = layout.slider.source_end_x - txt->w;
        r.y = layout.slider.source_y;
        r.w = txt->w;
        r.h = txt->h;
        SDL_BlitSurface(txt, 0, slider_surface, &r);
        SDL_FreeSurface(txt);
    }

    r.x = layout.slider.track_x;
    r.y = layout.slider.track_y;
    r.w = layout.slider.track_width;
    r.h = 3;
    SDL_FillRect(slider_surface, &r, SDL_MapRGB(slider_surface->format, 80, 80, 80));

    r.x = layout.slider.handle_start_x +
          param_state_get(state) * (layout.slider.track_width - layout.slider.handle_width);
    r.y = layout.slider.handle_y;
    r.w = layout.slider.handle_width;
    r.h = layout.slider.handle_height;


    SDL_FillRect(slider_surface, &r, SDL_MapRGB(slider_surface->format,
                                                handle_color.r,
                                                handle_color.g,
                                                handle_color.b));
}

void slider_output_render(param_output_t * output)
{
    SDL_Color handle_color = {255, 0, 0};

    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.output_slider.width;
    r.h = layout.output_slider.height;
    SDL_FillRect(output_slider_surface, &r, SDL_MapRGB(slider_surface->format, 20, 20, 20));

    r.x = layout.output_slider.track_x;
    r.y = layout.output_slider.track_y;
    r.w = layout.output_slider.track_width;
    r.h = layout.output_slider.track_height;
    SDL_FillRect(output_slider_surface, &r, SDL_MapRGB(slider_surface->format, 80, 80, 80));

    r.x = layout.output_slider.handle_start_x +
          output->value * (layout.output_slider.track_width - layout.output_slider.handle_width);
    r.y = layout.output_slider.handle_y;
    r.w = layout.output_slider.handle_width;
    r.h = layout.output_slider.handle_height;


    SDL_FillRect(output_slider_surface, &r, SDL_MapRGB(slider_surface->format,
                                                       handle_color.r,
                                                       handle_color.g,
                                                       handle_color.b));
}
