#include "core/err.h"
#include "ui/layout.h"
#include "ui/slider.h"
#include "ui/text.h"
#include "ui/ui.h"

#include <SDL/SDL_ttf.h>

SDL_Surface* slider_surface;
SDL_Surface* alpha_slider_surface;

static struct
{
    param_state_t * state;
    float initial_value;
} active_slider;

void slider_init()
{
    slider_surface = SDL_CreateRGBSurface(0, layout.slider.w, layout.slider.h, 32, 0, 0, 0, 0);
    if(!slider_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    alpha_slider_surface = SDL_CreateRGBSurface(0, layout.alpha_slider.w, layout.alpha_slider.h, 32, 0, 0, 0, 0);
    if(!slider_surface) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
}

void slider_del()
{
    SDL_FreeSurface(slider_surface);
}

void slider_render_alpha(param_state_t* state)
{
    param_output_t * param_output = param_state_output(state);
    SDL_Color handle_color = {0, 0, 80, 255};
    SDL_Rect r;

    if(param_output){
        handle_color = param_output->handle_color;
    }else{

    }

    SDL_FillRect(alpha_slider_surface, &layout.alpha_slider.rect, SDL_MapRGB(slider_surface->format, 20, 20, 20));

    SDL_FillRect(alpha_slider_surface, &layout.alpha_slider.track_rect, SDL_MapRGB(slider_surface->format, 80, 80, 80));

    rect_copy(&r, &layout.alpha_slider.handle_rect);
    r.y += (1.0 - param_state_get(state)) * (layout.alpha_slider.track_h - layout.alpha_slider.handle_h);
    SDL_FillRect(alpha_slider_surface, &r, SDL_MapRGB(alpha_slider_surface->format,
                                                handle_color.r,
                                                handle_color.g,
                                                handle_color.b));
}

void slider_render(parameter_t* param, param_state_t* state, SDL_Color c)
{
    param_output_t * param_output = param_state_output(state);
    SDL_Color handle_color = {0, 0, 80, 255};
    SDL_Color white = {255, 255, 255, 255};

    SDL_Rect r;
    SDL_FillRect(slider_surface, &layout.slider.rect, SDL_MapRGB(slider_surface->format, 20, 20, 20));

    text_render(slider_surface, &layout.slider.name_txt, &c, param->name);

    if(param->val_to_str){
        char sbuf[129];
        param->val_to_str(param_state_get(state), sbuf, 128);
        text_render(slider_surface, &layout.slider.value_txt, &white, sbuf);
    }

    if(param_output){
        handle_color = param_output->handle_color;

        text_render(slider_surface, &layout.slider.source_txt, &param_output->label_color, param_output->label);
    }

    SDL_FillRect(slider_surface, &layout.slider.track_rect, SDL_MapRGB(slider_surface->format, 80, 80, 80));

    rect_copy(&r, &layout.slider.handle_rect);
    r.x += param_state_get(state) * (layout.slider.track_w - layout.slider.handle_w);
    SDL_FillRect(slider_surface, &r, SDL_MapRGB(slider_surface->format,
                                                handle_color.r,
                                                handle_color.g,
                                                handle_color.b));
}

void mouse_drag_alpha_slider(struct xy xy)
{
    float val = active_slider.initial_value -
                (float)xy.y / (layout.alpha_slider.track_h - layout.alpha_slider.handle_h);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    param_state_setq(active_slider.state, val);
}

void mouse_drag_param_slider(struct xy xy)
{
    float val = active_slider.initial_value +
                (float)xy.x / (layout.slider.track_w - layout.slider.handle_w);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    param_state_setq(active_slider.state, val);
}

enum slider_event mouse_click_alpha_slider(param_state_t * param_state, struct xy xy){
    rect_t r;
    struct xy offset;

    rect_copy(&r, &layout.alpha_slider.handle_rect);
    r.y += (1.0 - param_state_get(param_state)) * (layout.alpha_slider.track_h - layout.alpha_slider.handle_h);
    if(xy_in_rect(&xy, &r, &offset)){
        active_slider.state = param_state;
        active_slider.initial_value = param_state_get(param_state);
        mouse_drag_fn_p = &mouse_drag_alpha_slider;
        return SLIDER_DRAG_START;
    }
    return 0;
}

enum slider_event mouse_click_param_slider(param_state_t * param_state, struct xy xy){
    rect_t r;
    struct xy offset;

    rect_copy(&r, &layout.slider.handle_rect);
    r.x += param_state_get(param_state) * (layout.slider.track_w - layout.slider.handle_w);
    if(xy_in_rect(&xy, &r, &offset)){
        if(param_state->connected_output)
            return SLIDER_LOCKED;
        active_slider.state = param_state;
        active_slider.initial_value = param_state_get(param_state);
        mouse_drag_fn_p = &mouse_drag_param_slider;
        return SLIDER_DRAG_START;
    }

    r.x = 0;
    r.y = 0;
    r.w = layout.slider.track_w; // FIXME
    r.h = layout.slider.handle_y - layout.slider.source_y;
    if(xy_in_rect(&xy, &r, &offset)){
        if(active_param_source)
            param_state_disconnect(active_param_source);
        if(active_param_source == param_state){
            active_param_source = 0;
            return SLIDER_CONNECT_CANCEL;
        }else{
            active_param_source = param_state;
            return SLIDER_CONNECT_START;
        }
    }
    return 0;
}
