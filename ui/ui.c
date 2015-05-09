#include <math.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

#include "ui/ui.h"
#include "ui/layout.h"
#include "ui/slider.h"
#include "core/err.h"
#include "core/parameter.h"
#include "core/slot.h"
#include "filters/filter.h"
#include "hits/hit.h"
#include "midi/midi.h"
#include "midi/layout.h"
#include "output/slice.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "timebase/timebase.h"

#define SURFACES \
    X(screen, window) \
    X(master_preview, master) \
    X(pattern_preview, pattern) \
    X(slot_pane, slot) \
    X(hit_slot_pane, hit_slot) \
    X(pattern_pane, pattern) \
    X(hit_pane, hit) \
    X(signal_pane, signal) \
    X(filter_pane, filter)

#define X(s, l) \
    static SDL_Surface* s;
SURFACES
#undef X

static TTF_Font* pattern_font;
static TTF_Font* signal_font;
static TTF_Font* filter_font;

static int mouse_down;
static int mouse_drag_start_x;
static int mouse_drag_start_y;

static void (*mouse_drag_fn_p)(int x, int y);
static void (*mouse_drop_fn_p)();

static struct
{
    param_state_t * state;
    float initial_value;
} active_slider;

static struct
{
    int index;
    struct xy dxy;
} active_pattern;

static struct
{
    int index;
    struct xy dxy;
} active_hit;

static struct
{
    slot_t * slot;
    struct xy dxy;
    int is_pattern;
} active_slot;

enum slider_event {
    SLIDER_LOCKED = 1,
    SLIDER_DRAG_START,
    SLIDER_DRAG_END,
    SLIDER_CONNECT_START,
    SLIDER_CONNECT_CANCEL,
    SLIDER_CONNECT,
};

static struct active_hit * active_active_hit;

param_state_t * active_param_source;

void ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        FAIL("SDL_Init Error: %s\n", SDL_GetError());
    }

    if (TTF_Init())
    {
        FAIL("TTF_Init Error: %s\n", SDL_GetError());
    }

    screen = SDL_SetVideoMode(layout.window.w, layout.window.h, 0, SDL_DOUBLEBUF);

    if (!screen) FAIL("SDL_SetVideoMode Error: %s\n", SDL_GetError());

#define X(s, l) \
    s = SDL_CreateRGBSurface(0, layout.## l ##.w, layout.## l ##.h, 32, 0, 0, 0, 0); \
    if(!s) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
SURFACES
#undef X

    pattern_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 20);
    if(!pattern_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    signal_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 20);
    if(!signal_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    filter_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 14);
    if(!filter_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    slider_init();
    graph_init();

    mouse_down = 0;
    mouse_drag_fn_p = 0;
    mouse_drop_fn_p = 0;
    active_pattern.index = -1;
    active_slot.slot = 0;
}

void ui_quit()
{
    for(int i=0; i<n_slots; i++)
    {
        pat_unload(&slots[i]);
    }

#define X(s, l) \
    SDL_FreeSurface(s);
SURFACES
#undef X

    TTF_CloseFont(pattern_font);
    TTF_CloseFont(signal_font);
    TTF_CloseFont(filter_font);

    slider_del();
    graph_del();

    SDL_Quit();
}

static int x_to_px(float x)
{
    return (int)(((x + 1) / 2) * layout.master.w);
}

static int y_to_px(float y)
{
    return (int)(((y + 1) / 2) * layout.master.h);
}

static int SDL_line(SDL_Surface* dst, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if(x1 == x2) return vlineRGBA(dst, x1, y1, y2, r, g, b, a);
    if(y1 == y2) return hlineRGBA(dst, x1, x2, y1, r, g, b, a);
    return lineRGBA(dst, x1, y1, x2, y2, r, g, b, a);
}

static void update_master_preview()
{
    SDL_LockSurface(master_preview);

    for(int x=0;x<layout.master.w;x++)
    {
        for(int y=0;y<layout.master.h;y++)
        {
            float xf = ((float)x / (layout.master.w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.master.h - 1)) * 2 - 1;
            color_t pixel = render_composite(xf, yf);
            ((uint32_t*)(master_preview->pixels))[x + layout.master.w * y] = SDL_MapRGB(
                master_preview->format,
                (uint8_t)roundf(255 * pixel.r),
                (uint8_t)roundf(255 * pixel.g),
                (uint8_t)roundf(255 * pixel.b));
        }
    }

    SDL_UnlockSurface(master_preview);

    for(int i=0; i<n_output_strips; i++)
    {
        int x1;
        int y1;
        output_vertex_t* v = output_strips[i].first;

        int x2 = x_to_px(v->x);
        int y2 = y_to_px(v->y);
        for(v = v->next; v; v = v->next)
        {
            x1 = x2;
            y1 = y2;
            x2 = x_to_px(v->x);
            y2 = y_to_px(v->y);

            SDL_line(master_preview,
                     x1, y1,
                     x2, y2,
                     255,255,0,255);
        }
    }
}

static void update_pattern_preview(slot_t* slot)
{
    SDL_LockSurface(pattern_preview);

    for(int x = 0; x < layout.slot.preview.w; x++)
    {
        for(int y = 0; y < layout.preview.h; y++)
        {
            float xf = ((float)x / (layout.slot.preview.w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.slot.preview.h - 1)) * 2 - 1;
            color_t pixel = (*slot->pattern->render)(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.slot.preview.w * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * pixel.r * pixel.a),
                (uint8_t)roundf(255 * pixel.g * pixel.a),
                (uint8_t)roundf(255 * pixel.b * pixel.a));
        }
    }

    SDL_UnlockSurface(pattern_preview);
}

static void update_hit_preview(slot_t* slot)
{
    SDL_LockSurface(pattern_preview);

    for(int x = 0; x < layout.hit_slot.preview.w; x++)
    {
        for(int y = 0; y < layout.hit_slot.preview.h; y++)
        {
            float xf = ((float)x / (layout.hit_slot.preview.w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.hit_slot.preview.h - 1)) * 2 - 1;
            color_t pixel = render_composite_slot_hits(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.hit_slot.preview.w * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * pixel.r * pixel.a),
                (uint8_t)roundf(255 * pixel.g * pixel.a),
                (uint8_t)roundf(255 * pixel.b * pixel.a));
        }
    }

    SDL_UnlockSurface(pattern_preview);
}

static void ui_update_slot(slot_t* slot)
{
    union rect r;
    SDL_Color param_name_c = {255, 255, 255};
    rect_copy(&r, &layout.slot);
    SDL_FillRect(slot_pane, &r, SDL_MapRGB(slot_pane->format, 20, 20, 20));

    if(slot->pattern)
    {
        update_pattern_preview(slot);
        SDL_BlitSurface(pattern_preview, 0, slot_pane, &layout.preview.rect.sdl);

        SDL_Color alpha_color = {0, 0, 80};
        if(param_state_output(&slot->alpha)){
            alpha_color = param_state_output(&slot->alpha)->handle_color;
        }
        SDL_FillRect(slot_pane, &layout.alpha_slider.rect.sdl, SDL_MapRGB(pattern_preview->format, 80, 80, 80));

        rect_copy(&r, &layout.alpha_handle.rect);
        r.y += (1 - param_state_get(&hit_slot->alpha)) * (layout.alpha_slider.h - layout.alpha_handle.h);

        SDL_FillRect(slot_pane, &r, SDL_MapRGB(slot_pane->format,
                                               alpha_color.r,
                                               alpha_color.g,
                                               alpha_color.b));

        for(int i = 0; i < slot->pattern->n_params; i++)
        {
            if(&slot->param_states[i] == active_param_source){
                //param_name_c = {255, 200, 0};
                param_name_c.r = 255;
                param_name_c.g = 200;
                param_name_c.b =   0;
            }else{
                //param_name_c = {255, 255, 255};
                param_name_c.r = 255;
                param_name_c.g = 255;
                param_name_c.b = 255;
            }
            slider_render(&slot->pattern->parameters[i], &slot->param_states[i], param_name_c);
            rect_array_layout(&layout.pattern_slider.rect_array, i, &r);

            SDL_BlitSurface(slider_surface, 0, slot_pane, &r);
        }
        
    }
}

static void ui_update_hit_slot(slot_t* hit_slot)
{
    union rect r;
    SDL_Color param_name_c = {255, 255, 255};
    rect_copy(&r, &layout.hit_slot.rect_array);
    SDL_FillRect(hit_slot_pane, &r.sdl, SDL_MapRGB(slot_pane->format, 20, 20, 20));

    if(hit_slot->hit)
    {
        update_hit_preview(hit_slot);
        SDL_BlitSurface(pattern_preview, 0, hit_slot_pane, &layout.preview.rect.sdl);

        SDL_Color alpha_color = {0, 0, 80};
        if(param_state_output(&hit_slot->alpha)){
            alpha_color = param_state_output(&hit_slot->alpha)->handle_color;
        }
        SDL_FillRect(hit_slot_pane, &layout.alpha_slider.rect.sdl, SDL_MapRGB(hit_slot_pane->format, 80, 80, 80));
        rect_copy(&r, &layout.alpha_handle.rect);
        r.y += (1 - param_state_get(&hit_slot->alpha)) * (layout.alpha_slider.h - layout.alpha_handle.h);

        SDL_FillRect(hit_slot_pane, &r, SDL_MapRGB(hit_slot_pane->format,
                                                   alpha_color.r,
                                                   alpha_color.g,
                                                   alpha_color.b));

        for(int i = 0; i < hit_slot->hit->n_params; i++)
        {
            if(&hit_slot->param_states[i] == active_param_source){
                //param_name_c = {255, 200, 0};
                param_name_c.r = 255;
                param_name_c.g = 200;
                param_name_c.b =   0;
            }else{
                //param_name_c = {255, 255, 255};
                param_name_c.r = 255;
                param_name_c.g = 255;
                param_name_c.b = 255;
            }
            slider_render(&hit_slot->hit->parameters[i], &hit_slot->param_states[i], param_name_c);
            rect_array_layout(&layout.pattern_slider.rect_array, i, &r);

            SDL_BlitSurface(slider_surface, 0, hit_slot_pane, &r);
        }
        
    }
}

static void ui_update_filter(filter_t * filter)
{
    union rect r;
    rect_copy(&r, &layout.filter.rect_array);
    SDL_FillRect(filter_pane, &r, SDL_MapRGB(filter_pane->format, 20, 20, 20));

    SDL_Color white = {255, 255, 255};
    SDL_Surface* msg = TTF_RenderText_Solid(filter_font, filter->name, white);
    r.x = layout.filter.text_x;
    r.y = layout.filter.text_y;

    r.w = msg->w;
    r.h = msg->h;
    SDL_BlitSurface(msg, 0, filter_pane, &r);

    graph_update(&(filter->graph_state), filter->output.value);
    graph_render(&(filter->graph_state), white);
    SDL_BlitSurface(graph_surface, 0, filter_pane, &layout.graph_filter.rect.sdl);

    SDL_FreeSurface(msg);
}

static void ui_update_pattern(pattern_t* pattern)
{
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.add_pattern.w;
    r.h = layout.add_pattern.h;
    SDL_FillRect(pattern_pane, &r, SDL_MapRGB(pattern_pane->format, 20, 20, 20));

    SDL_Color white = {255, 255, 255};
    SDL_Surface* msg = TTF_RenderText_Solid(pattern_font, pattern->name, white);
    r.x = layout.add_pattern.text_x;
    r.y = layout.add_pattern.text_y;

    r.w = msg->w;
    r.h = msg->h;
    SDL_BlitSurface(msg, 0, pattern_pane, &r);
    SDL_FreeSurface(msg);
}

static void ui_update_hit(hit_t * hit)
{
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.add_hit.w;
    r.h = layout.add_hit.h;
    SDL_FillRect(hit_pane, &r, SDL_MapRGB(hit_pane->format, 20, 20, 20));

    SDL_Color white = {255, 255, 255};
    SDL_Surface* msg = TTF_RenderText_Solid(pattern_font, hit->name, white);
    r.x = layout.add_hit.text_x;
    r.y = layout.add_hit.text_y;

    r.w = msg->w;
    r.h = msg->h;
    SDL_BlitSurface(msg, 0, hit_pane, &r);
    SDL_FreeSurface(msg);
}

static void ui_update_signal(signal_t* signal)
{
    union rect r;
    SDL_Color param_name_c;
    SDL_FillRect(signal_pane, &layout.signal.rect.sdl, SDL_MapRGB(signal_pane->format, 20, 20, 20));

    SDL_Surface* msg = TTF_RenderText_Solid(signal_font, signal->name, color_to_SDL(signal->color));
    r.x = layout.signal.text_x;
    r.y = layout.signal.text_y;

    r.w = msg->w;
    r.h = msg->h;
    SDL_BlitSurface(msg, 0, signal_pane, &r.sdl);
    SDL_FreeSurface(msg);

    graph_update(&(signal->graph_state), signal->output.value);
    SDL_Color white = {255, 255, 255};
    graph_render(&(signal->graph_state), white);
    SDL_BlitSurface(graph_surface, 0, signal_pane, &layout.graph_signal.rect.sdl);

    for(int i = 0; i < signal->n_params; i++)
    {
        if(&signal->param_states[i] == active_param_source){
            //param_name_c = {255, 200, 0};
            param_name_c.r = 255;
            param_name_c.g = 200;
            param_name_c.b =   0;
        }else{
            //param_name_c = {255, 255, 255};
            param_name_c.r = 255;
            param_name_c.g = 255;
            param_name_c.b = 255;
        }
        slider_render(&signal->parameters[i], &signal->param_states[i], param_name_c);
        rect_array_layout(&layout.signal_slider, i, &r);

        SDL_BlitSurface(slider_surface, 0, signal_pane, &r);
    }
}

void ui_render()
{
    union rect r;
    SDL_FillRect(screen, &layout.window.rect.sdl, SDL_MapRGB(screen->format, 0, 0, 0));

    update_master_preview();
    SDL_BlitSurface(master_preview, 0, screen, &layout.master.rect.sdl);

    for(int i = 0; i < n_signals; i++)
    {
        ui_update_signal(&signals[i]);
        rect_array_layout(&layout.signal_pane.rect_array, i, &r);
        SDL_BlitSurface(signal_pane, 0, screen, &r.sdl);
    }

    for(int i=0; i<n_slots; i++)
    {
        if(slots[i].pattern)
        {
            ui_update_slot(&slots[i]);
            rect_array_layout(&layout.slot_pane.rect_array, i, &r);

            if(active_slot.slot == &slots[i]) {
                rect_translate(&r, &active_slot.dxy);
            }

            SDL_BlitSurface(slot_pane, 0, screen, &r.sdl);
        }
    }

    for(int i=0; i<n_hit_slots; i++)
    {
        if(hit_slots[i].hit)
        {
            ui_update_hit_slot(&hit_slots[i]);
            rect_array_layout(&layout.hit_slot_pane.rect_array, i, &r);

            if(active_slot.slot == &hit_slots[i]) { //TODO ? shoudl this be active_hit_slot
                rect_translate(&r, &active_slot.dxy);
            }

            SDL_BlitSurface(hit_slot_pane, 0, screen, &r.sdl);
        }
    }

    for(int i=0; i<n_patterns; i++)
    {
        ui_update_pattern(patterns[i]);
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);

        if(active_pattern.index == i) {
            rect_translate(&r, &active_pattern.dxy);
        }

        SDL_BlitSurface(pattern_pane, 0, screen, &r.sdl);
    }

    for(int i=0; i<n_hits; i++)
    {
        ui_update_hit(hits[i]);
        rect_array_layout(&layout.add_hit.rect_array, i, &r);

        if(active_hit.index == i) {
            rect_translate(&r, &active_hit.dxy);
        }

        SDL_BlitSurface(hit_pane, 0, screen, &r.sdl);
    }

    for(int i = 0; i < n_filters; i++){
        ui_update_filter(&filters[i]);
        rect_array_layout(&layout.filter.rect_array, i, &r);

        SDL_BlitSurface(filter_pane, 0, screen, &r.sdl);
    }

    SDL_Flip(screen);
}

static void mouse_drag_alpha_slider(int x, int y)
{
    float val = active_slider.initial_value -
                (float)y / (layout.alpha_slider.track.h - layout.alpha_slider.handle.h);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    param_state_setq(&active_slider.state, val);
}

static void mouse_drag_param_slider(int x, int y)
{
    float val = active_slider.initial_value +
                (float)x / (layout.slider.track.w - layout.slider.handle.w);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    param_state_setq(&active_slider.state, val);
}

static void mouse_drag_pattern(int x, int y)
{
    active_pattern.dx = x;
    active_pattern.dy = y;
}

static void mouse_drag_slot(int x, int y)
{
    active_slot.dx = x;
    active_slot.dy = y;
}


static void mouse_drop_pattern()
{
    union rect r;
    struct xy xy;
    xy.x = mouse_drag_start_x + active_pattern.dx;
    xy.y = mouse_drag_start_y + active_pattern.dy;

    for(int i = 0; i < n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(in_rect(&xy, &r, 0)){
            pat_unload(&slots[i]);
            pat_load(&slots[i],patterns[active_pattern.index]);
        }
    }
    active_pattern.index = -1;
}

static void mouse_drop_slot()
{
    union rect r;
    struct xy xy;

    if(active_slot.is_pattern){
        for(int i = 0; i < n_slots; i++)
        {
            rect_array_layout(&layout.slot.rect_array, i, &r);
            if(in_rect(&xy, &r, 0)){
                slot_t temp_slot = *active_slot.slot;
                *active_slot.slot = slots[i];
                slots[i] = temp_slot;
            }
        }
    }
    active_slot.slot = 0;
}

static enum slider_event mouse_click_alpha_slider(param_state_t * param_state, struct xy){
    union rect r;
    struct xy offset;

    rect_copy(&r, &layout.alpha_slider.handle_rect);
    r.y += param_state_get(param_state) * (layout.alpha_slider.track.h - layout.alpha_slider.handle.h);
    if(in_rect(&xy, &r, &offset)){
        active_alpha_slider.state = param_state;
        active_alpha_slider.initial_value = param_state_get(param_state);
        mouse_drag_fn_p = &mouse_drag_alpha_slider;
        return SLIDER_DRAG_START;
    }
}

static enum slider_event mouse_click_slider(param_state_t * param_state, struct xy){
    union rect r;
    struct xy offset;

    rect_copy(&r, &layout.slider.handle_rect);
    r.x += param_state_get(param_state) * (layout.slider.track.w - layout.slider.handle.w);
    if(in_rect(&xy, &r, &offset)){
        if(param_state->connected_output)
            return SLIDER_LOCKED;
        active_param_slider.state = param_state;
        active_param_slider.initial_value = param_state_get(param_state);
        mouse_drag_fn_p = &mouse_drag_param_slider;
        return SLIDER_DRAG_START;
    }

    rect_copy(&r, &r_slider);
    r.w = layout.slider.track.w; // FIXME
    r.h = layout.signal_slider.handle.y - layout.signal_slider.source.y;
    if(in_rect(&xy, &r, &offset)){
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
}

static int mouse_click_slot(int index, struct xy xy)
{
    union rect r;
    struct xy offset;
    if(!slots[index].pattern) return 0;

    // See if the click is on the alpha slider
    if(in_rect(&xy, &layout.slot.alpha.rect, &offset)){
        return !!mouse_click_alpha_slider(&slots[index].alpha, offset);
    }

    // See if the click is on the preview 
    if(in_rect(&xy, &layout.slot.preview.rect, &offset)){
        slots[index].pattern->prevclick(
                &slots[index],
                -1.0 + 2.0 * (xy.x - layout.slot.preview_x) / (float) layout.preview_w,
                -1.0 + 2.0 * (xy.y - layout.slot.preview_y) / (float) layout.preview_h);
        return 1; 
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < slots[index].pattern->n_params; i++)
    {
        rect_array_layout(&layout.slot.sliders.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&slots[index].param_states[i], offset);
        }
    }

    // Else, drag the slot
    active_slot.slot = &slots[index];
    active_slot.is_pattern = 1;
    active_slot.dx = 0;
    active_slot.dy = 0;

    mouse_drag_fn_p = &mouse_drag_slot;
    mouse_drop_fn_p = &mouse_drop_slot;
    return 1;

}

static void hit_release(){
    if(active_active_hit && active_active_hit->hit){
        active_active_hit->hit->event(active_active_hit, HITEV_NOTE_OFF, 1.);
    }
    active_active_hit = 0;
}

static int mouse_click_hit_slot(int index, struct xy xy)
{
    union rect r;
    struct xy offset;

    if(!hit_slots[index].hit) return 0;

    // See if the click is on the alpha slider
    if(in_rect(&xy, &layout.hit_slot.alpha.rect, &offset)){
        return !!mouse_click_alpha_slider(&hit_slots[index].alpha, offset);
    }

    // See if the click is on the preview 
    if(in_rect(&xy, &layout.hit_slot.preview.rect, &offset)){
        active_active_hit = hit_slots[index].hit->start(&hit_slots[index]);
        if(active_active_hit) active_active_hit->hit->event(active_active_hit, HITEV_NOTE_ON, 1.);
        hit_slots[index].hit->prevclick(
                &hit_slots[index],
                -1.0 + 2.0 * (xy.x - layout.hit_slot.preview_x) / (float) layout.hit_slot.preview_w,
                -1.0 + 2.0 * (xy.y - layout.hit_slot.preview_y) / (float) layout.hit_slot.preview_h);
        return 1; 
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < hit_slots[index].hit->n_params; i++)
    {
        rect_array_layout(&layout.hit_slot.sliders.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&hit_slots[index].param_states[i], offset);
        }
    }

    // Else, drag the slot
    active_slot.slot = &hit_slots[index];
    active_slot.is_pattern = 0;
    active_slot.dx = 0;
    active_slot.dy = 0;

    mouse_drag_fn_p = &mouse_drag_slot;
    mouse_drop_fn_p = &mouse_drop_slot;
    return 1;

}

static int mouse_click_signal(int index, struct xy xy)
{
    union rect r;
    struct xy offset;
    for(int i = 0; i < signals[index].n_params; i++)
    {
        // See if the click is on a parameter slider
        rect_array_layout(&layout.signal_slider.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&signals[index].param_states[i], offset);
        }
    }

    // Was the output clicked
    if(in_rect(&xy, &layout.graph_signal, 0)){
        // Is there something looking for a source?
        if(active_param_source){
            param_state_connect(active_param_source, &signals[index].output);
            active_param_source = 0;
        }
    }

    return 0;
}

static int mouse_click_filter(int index, struct xy xy){
    if(in_rect(&xy, &layout.graph_filter, 0)){
        if(active_param_source){
            param_state_connect(active_param_source, &filters[index].output);
            active_param_source = 0;
            return 1;
        }
    }
    return 0;
}

static int mouse_click(struct xy xy)
{
    struct xy offset;
    union rect r;
    // See if click is in a slot
    for(int i=0; i<n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            // If it is, see if that slot wants to handle it
            return mouse_click_slot(i, offset);
        }
    }

    // See if click is in a hit slot
    for(int i=0; i<n_hit_slots; i++)
    {
        rect_array_layout(&layout.hit_slot.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            // If it is, see if that slot wants to handle it
            return mouse_click_hit_slot(i, offset);
        }
    }

    // See if click is in a pattern
    for(int i=0; i<n_slots; i++)
    {
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            active_pattern.index = i;
            active_pattern.dxy = (struct xy) {0,0};
            mouse_drag_fn_p = &mouse_drag_pattern;
            mouse_drop_fn_p = &mouse_drop_pattern;
            return 1;
        }
    }

    // See if click is in an signal
    for(int i = 0; i < n_signals; i++){
        rect_array_layout(&layout.signal.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return mouse_click_signal(i, offset);
        }
    }

    // See if click is in filter
    for(int i = 0; i < n_filters; i++){
        rect_array_layout(&layout.filter.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return mouse_click_filter(i, offset);
        }
    }

    // Otherwise, do not handle click
    // TEMP: treat click as beat tap
    timebase_tap(0.8);
    return 1;
    //return 0;
}

int ui_poll()
{
    SDL_Event e;
    struct midi_event * me;
    while(SDL_PollEvent(&e)) 
    {
        switch(e.type)
        {
            case SDL_QUIT:
                return 0;
            case SDL_MOUSEBUTTONDOWN:
                mouse_click(e.button.x, e.button.y);
                mouse_down = 1;
                mouse_drag_start_x = e.button.x;
                mouse_drag_start_y = e.button.y;
                break;
            case SDL_MOUSEMOTION:
                if(mouse_down && mouse_drag_fn_p)
                {
                    (*mouse_drag_fn_p)(e.button.x - mouse_drag_start_x,
                                    e.button.y - mouse_drag_start_y);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(mouse_drop_fn_p) (*mouse_drop_fn_p)();
                mouse_drag_fn_p = 0;
                mouse_drop_fn_p = 0;
                mouse_down = 0;
                break;
            case SDL_MIDI_NOTE_ON:
                me = e.user.data1;
                printf("Note on: %d %d %d %d\n", me->device, me->event, me->data1, me->data2);
                midi_handle_note_event(me);
                free(me);
                break;
            case SDL_MIDI_NOTE_OFF:
                me = e.user.data1;
                printf("Note off: %d %d %d %d\n", me->device, me->event, me->data1, me->data2);
                midi_handle_note_event(me);
                free(me);
                break;
        }
    }
    return 1;
}

