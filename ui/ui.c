#include <math.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

#include "ui/ui.h"
#include "ui/slider.h"
#include "core/err.h"
#include "core/slot.h"
#include "output/slice.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "core/parameter.h"
#include "filters/filter.h"
#include "timebase/timebase.h"

static SDL_Surface* screen;
static SDL_Surface* master_preview;
static SDL_Surface* pattern_preview;
static SDL_Surface* slot_pane;
static SDL_Surface* pattern_pane;
static SDL_Surface* signal_pane;
static SDL_Surface* filter_pane;
static TTF_Font* pattern_font;
static TTF_Font* signal_font;
static TTF_Font* filter_font;

static int mouse_down;
static int mouse_drag_start_x;
static int mouse_drag_start_y;

layout_t layout = {
    //.win_width = 1366,
    //.win_height = 768,
    .win_width = 1200,
    .win_height = 700,

    .master_x = 30,
    .master_y = 30,
    .master_width = 200,
    .master_height = 200,

    .slot_start_x = 10,
    .slot_start_y = 250,
    .slot_width = 110,
    .slot_height = 250,
    .slot_pitch = 125,

    .preview_x = 25,
    .preview_y = 5,
    .preview_width = 80,
    .preview_height = 80,

    .alpha_slider_x = 9,
    .alpha_slider_y = 5,
    .alpha_slider_height = 80,
    .alpha_handle_x = 5,
    .alpha_handle_y = 5,
    .alpha_handle_width = 10,
    .alpha_handle_height = 10,

    .pattern = {
        .slider_start_x = 0,
        .slider_start_y = 90,
        .slider_pitch = 34,
    },

    .slider = {
        .width = 100,
        .height = 40,
        .name_x = 3,
        .name_y = 0,
        .source_end_x = 94,
        .source_y = 3,
        .track_x = 3,
        .track_y = 16,
        .track_width = 94,
        .handle_start_x = 3,
        .handle_y = 12,
        .handle_width = 10,
        .handle_height = 10, 
        .value_start_x = 0,
        .value_start_y = 22,
    },

    .pattern_start_x = 10,
    .pattern_start_y = 520,
    .pattern_width = 100,
    .pattern_height = 30,
    .pattern_pitch = 120,
    .pattern_text_x = 5,
    .pattern_text_y = 3,

    .signal = {
        .start_x = 600,
        .start_y = 30,
        .width = 110,
        .height = 200,
        .pitch = 125,
        .text_x = 5,
        .text_y = 3,
        .output_x = 3,
        .output_y = 30,
        .slider_start_x = 3,
        .slider_start_y = 65,
        .slider_pitch = 35,
    },

    .graph = {
        .width = 100,
        .height = 30,
        .scroll_rate = 50,
    },

    .filter = {
        .start_x = 380,
        .start_y = 30,
        .width = 200,
        .height = 200,
        .pitch_x = 0,
        .pitch_y = 60,
        .text_x = 5,
        .text_y = 5,
        .waveform_x = 5,
        .waveform_y = 30,
        .waveform_width = 190,
        .waveform_height = 40,
    },

};

static void (*mouse_drag_fn_p)(int x, int y);
static void (*mouse_drop_fn_p)();

static struct
{
    slot_t* slot;
    float initial_value;
} active_alpha_slider;

static struct
{
    param_state_t * state;
    float initial_value;
} active_param_slider;

static struct
{
    int index;
    int dx;
    int dy;
} active_pattern;

static struct
{
    int index;
    int dx;
    int dy;
} active_slot;

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

    screen = SDL_SetVideoMode(layout.win_width, layout.win_height, 0, SDL_DOUBLEBUF);

    if (!screen) FAIL("SDL_SetVideoMode Error: %s\n", SDL_GetError());

    master_preview = SDL_CreateRGBSurface(0, layout.master_width, layout.master_height, 32, 0, 0, 0, 0);

    if(!master_preview) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    pattern_preview = SDL_CreateRGBSurface(0, layout.preview_width, layout.preview_height, 32, 0, 0, 0, 0);
    if(!pattern_preview) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    slot_pane = SDL_CreateRGBSurface(0, layout.slot_width, layout.slot_height, 32, 0, 0, 0, 0);
    if(!slot_pane) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    pattern_pane = SDL_CreateRGBSurface(0, layout.pattern_width, layout.pattern_height, 32, 0, 0, 0, 0);
    if(!pattern_pane) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    signal_pane = SDL_CreateRGBSurface(0, layout.signal.width, layout.signal.height, 32, 0, 0, 0, 0);
    if(!signal_pane) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    filter_pane = SDL_CreateRGBSurface(0, layout.filter.width, layout.filter.height, 32, 0, 0, 0, 0);
    if(!filter_pane) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

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
    active_slot.index = -1;
}

void ui_quit()
{
    for(int i=0; i<n_slots; i++)
    {
        pat_unload(&slots[i]);
    }

    SDL_FreeSurface(master_preview);
    SDL_FreeSurface(pattern_preview);
    SDL_FreeSurface(slot_pane);
    SDL_FreeSurface(signal_pane);
    TTF_CloseFont(pattern_font);

    slider_del();
    graph_del();

    SDL_Quit();
}

static int x_to_px(float x)
{
    return (int)(((x + 1) / 2) * layout.master_width);
}

static int y_to_px(float y)
{
    return (int)(((y + 1) / 2) * layout.master_height);
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

    for(int x=0;x<layout.master_width;x++)
    {
        for(int y=0;y<layout.master_height;y++)
        {
            float xf = ((float)x / (layout.master_width - 1)) * 2 - 1;
            float yf = ((float)y / (layout.master_height - 1)) * 2 - 1;
            color_t pixel = render_composite(xf, yf);
            ((uint32_t*)(master_preview->pixels))[x + layout.master_width * y] = SDL_MapRGB(
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

    for(int x = 0; x < layout.preview_width; x++)
    {
        for(int y = 0; y < layout.preview_height; y++)
        {
            float xf = ((float)x / (layout.preview_width - 1)) * 2 - 1;
            float yf = ((float)y / (layout.preview_height - 1)) * 2 - 1;
            color_t pixel = (*slot->pattern->render)(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.preview_width * y] = SDL_MapRGB(
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
    SDL_Rect r;
    SDL_Color param_name_c = {255, 255, 255};
    r.x = 0;
    r.y = 0;
    r.w = layout.slot_width;
    r.h = layout.slot_height;
    SDL_FillRect(slot_pane, &r, SDL_MapRGB(slot_pane->format, 20, 20, 20));

    if(slot->pattern)
    {
        update_pattern_preview(slot);
        r.w = layout.preview_width;
        r.h = layout.preview_height;
        r.x = layout.preview_x;
        r.y = layout.preview_y;
        SDL_BlitSurface(pattern_preview, 0, slot_pane, &r);

        r.x = layout.alpha_slider_x;
        r.y = layout.alpha_slider_y;
        r.w = 3;
        r.h = layout.alpha_slider_height;
        SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 80, 80, 80));

        r.x = layout.alpha_handle_x;
        r.y = layout.alpha_handle_y +
              (1 - slot->alpha) * (layout.alpha_slider_height - layout.alpha_handle_height);
        r.w = layout.alpha_handle_width;
        r.h = layout.alpha_handle_height;

        SDL_FillRect(slot_pane, &r, SDL_MapRGB(slot_pane->format, 0, 0, 80));

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

            r.x = layout.pattern.slider_start_x;
            r.y = layout.pattern.slider_start_y + layout.pattern.slider_pitch * i;
            r.w = layout.slider.width;
            r.h = layout.slider.height;

            SDL_BlitSurface(slider_surface, 0, slot_pane, &r);
        }
        
    }
}

static void ui_update_filter(filter_t * filter)
{
    static float * history = 0;
    if(!history)
        history = malloc(sizeof(float) * layout.filter.waveform_width);
    if(!history)
        printf("Malloc failed"); //XXX

    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.filter.width;
    r.h = layout.filter.height;
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
    r.x = layout.filter.waveform_x;
    r.y = layout.filter.waveform_y;
    r.w = layout.graph.width;
    r.h = layout.graph.height;
    SDL_BlitSurface(graph_surface, 0, filter_pane, &r);

    /*
    filter->history(filter, history, layout.filter.waveform_width);
    float y_scale = 1e-6;
    for(int i = 0; i < layout.filter.waveform_width; i++){
        history[i] = fabs(history[i]);
        if(history[i] > y_scale) y_scale = history[i];
    }
    y_scale = ((float) layout.filter.waveform_height) / y_scale;
    SDL_LockSurface(filter_pane);
    Uint32 * pixels = (Uint32 *) filter_pane->pixels;
    for(int i = 0; i < layout.filter.waveform_width; i++){
        int v = history[i] * y_scale;
        int offset = (filter_pane->pitch / sizeof(Uint32)) * (layout.filter.waveform_y + layout.filter.waveform_height - v) + layout.filter.waveform_x + i;
        *(pixels + offset) = SDL_MapRGB(filter_pane->format, 200, 200, 200);
    }
    SDL_UnlockSurface(filter_pane);
    */
    SDL_FreeSurface(msg);
}

static void ui_update_pattern(pattern_t* pattern)
{
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.pattern_width;
    r.h = layout.pattern_height;
    SDL_FillRect(pattern_pane, &r, SDL_MapRGB(pattern_pane->format, 20, 20, 20));

    SDL_Color white = {255, 255, 255};
    SDL_Surface* msg = TTF_RenderText_Solid(pattern_font, pattern->name, white);
    r.x = layout.pattern_text_x;
    r.y = layout.pattern_text_y;

    r.w = msg->w;
    r.h = msg->h;
    SDL_BlitSurface(msg, 0, pattern_pane, &r);
    SDL_FreeSurface(msg);
}

static void ui_update_signal(signal_t* signal)
{
    SDL_Rect r;
    SDL_Color param_name_c;
    r.x = 0;
    r.y = 0;
    r.w = layout.signal.width;
    r.h = layout.signal.height;
    SDL_FillRect(signal_pane, &r, SDL_MapRGB(signal_pane->format, 20, 20, 20));

    SDL_Surface* msg = TTF_RenderText_Solid(signal_font, signal->name, color_to_SDL(signal->color));
    r.x = layout.signal.text_x;
    r.y = layout.signal.text_y;

    r.w = msg->w;
    r.h = msg->h;
    SDL_BlitSurface(msg, 0, signal_pane, &r);

    graph_update(&(signal->graph_state), signal->output.value);
    SDL_Color white = {255, 255, 255};
    graph_render(&(signal->graph_state), white);
    r.x = layout.signal.output_x;
    r.y = layout.signal.output_y;
    r.w = layout.graph.width;
    r.h = layout.graph.height;
    SDL_BlitSurface(graph_surface, 0, signal_pane, &r);

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

        r.x = layout.signal.slider_start_x;
        r.y = layout.signal.slider_start_y + layout.signal.slider_pitch * i;
        r.w = layout.slider.width;
        r.h = layout.slider.height;

        SDL_BlitSurface(slider_surface, 0, signal_pane, &r);
    }
}

void ui_render()
{
    SDL_Rect r;

    r.x = 0;
    r.y = 0;
    r.w = layout.win_width;
    r.h = layout.win_height;
    SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0, 0, 0));

    update_master_preview();
    r.x = layout.master_x;
    r.y = layout.master_y;
    r.w = layout.master_width;
    r.h = layout.master_height;
    SDL_BlitSurface(master_preview, 0, screen, &r);

    for(int i = 0; i < n_signals; i++)
    {
        ui_update_signal(&signals[i]);
        r.w = signal_pane->w;
        r.h = signal_pane->h;
        r.x = layout.signal.start_x + layout.signal.pitch * i;
        r.y = layout.signal.start_y;

        SDL_BlitSurface(signal_pane, 0, screen, &r);
    }

    for(int i=0; i<n_slots; i++)
    {
        if(slots[i].pattern)
        {
            ui_update_slot(&slots[i]);
            r.w = slot_pane->w;
            r.h = slot_pane->h;
            r.x = layout.slot_start_x + layout.slot_pitch * i;
            r.y = layout.slot_start_y;
            if(active_slot.index == i)
            {
                r.x += active_slot.dx;
                r.y += active_slot.dy;
            }

            SDL_BlitSurface(slot_pane, 0, screen, &r);
        }
    }

    for(int i=0; i<n_patterns; i++)
    {
        ui_update_pattern(patterns[i]);
        r.w = layout.pattern_width;
        r.h = layout.pattern_height;
        r.x = layout.pattern_start_x + layout.pattern_pitch * i;
        r.y = layout.pattern_start_y;
        if(active_pattern.index == i)
        {
            r.x += active_pattern.dx;
            r.y += active_pattern.dy;
        }

        SDL_BlitSurface(pattern_pane, 0, screen, &r);
    }

    for(int i = 0; i < n_filters; i++){
        ui_update_filter(&filters[i]);
        r.x = layout.filter.start_x + layout.filter.pitch_x * i;
        r.y = layout.filter.start_y + layout.filter.pitch_y * i;
        r.w = layout.filter.width;
        r.h = layout.filter.height;

        SDL_BlitSurface(filter_pane, 0, screen, &r);
    }

    SDL_Flip(screen);
}

static int in_rect(int x, int y, int rx, int ry, int rw, int rh)
{
    return x >= rx && y >= ry &&
           x < rx + rw && y < ry + rh;
}

static void mouse_drag_alpha_slider(int x, int y)
{
    float val = active_alpha_slider.initial_value -
                (float)y / (layout.alpha_slider_height - layout.alpha_handle_height);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    active_alpha_slider.slot->alpha = val;
}

static void mouse_drag_param_slider(int x, int y)
{
    float val = active_param_slider.initial_value +
                (float)x / (layout.slider.track_width - layout.slider.handle_width);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    active_param_slider.state->value = val;
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
    int x = mouse_drag_start_x + active_pattern.dx;
    int y = mouse_drag_start_y + active_pattern.dy;

    for(int i = 0; i < n_slots; i++)
    {
        if(in_rect(x,y,
                   layout.slot_start_x + layout.slot_pitch * i,
                   layout.slot_start_y,
                   layout.slot_width, layout.slot_height))
        {
            pat_unload(&slots[i]);
            pat_load(&slots[i],patterns[active_pattern.index]);
        }
    }
    active_pattern.index = -1;
}

static void mouse_drop_slot()
{
    int x = mouse_drag_start_x + active_slot.dx;
    int y = mouse_drag_start_y + active_slot.dy;

    for(int i = 0; i < n_slots; i++)
    {
        if(in_rect(x,y,
                   layout.slot_start_x + layout.slot_pitch * i,
                   layout.slot_start_y,
                   layout.slot_width, layout.slot_height))
        {
            slot_t temp_slot = slots[i];
            slots[i] = slots[active_slot.index];
            slots[active_slot.index] = temp_slot;
        }
    }
    active_slot.index = -1;
}

static int mouse_click_slot(int index, int x, int y)
{
    if(!slots[index].pattern) return 0;

    // See if the click is on the alpha slider
    if(in_rect(x, y,
               layout.alpha_handle_x,
               layout.alpha_handle_y +
                 (1 - slots[index].alpha) *
                 (layout.alpha_slider_height - layout.alpha_handle_height),
               layout.alpha_handle_width,
               layout.alpha_handle_height))
    {
        active_alpha_slider.slot = &slots[index];
        active_alpha_slider.initial_value = slots[index].alpha;
        mouse_drag_fn_p = &mouse_drag_alpha_slider;
        return 1;
    }

    // See if the click is on the preview 
    if(in_rect(x, y,
               layout.preview_x,
               layout.preview_y,
               layout.preview_width,
               layout.preview_height)){
        slots[index].pattern->prevclick(
                &slots[index],
                -1.0 + 2.0 * (x - layout.preview_x) / (float) layout.preview_width,
                -1.0 + 2.0 * (y - layout.preview_y) / (float) layout.preview_width);
        return 1; 
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < slots[index].pattern->n_params; i++)
    {
        if(in_rect(x, y,
                   layout.pattern.slider_start_x +
                   layout.slider.handle_start_x +
                     slots[index].param_states[i].value *
                     (layout.slider.track_width - layout.slider.handle_width),
                   layout.pattern.slider_start_y +
                   layout.slider.handle_y + layout.pattern.slider_pitch * i,
                   layout.slider.handle_width,
                   layout.slider.handle_height)) {
            if(slots[index].param_states[i].connected_output)
                return 1;
            active_param_slider.state = &slots[index].param_states[i];
            active_param_slider.initial_value = slots[index].param_states[i].value;
            mouse_drag_fn_p = &mouse_drag_param_slider;
            return 1;
        }
        // See if the click is on the parameter source button
        if(in_rect(x, y,
                   layout.pattern.slider_start_x +
                   layout.slider.name_x,
                   layout.pattern.slider_start_y +
                   layout.slider.name_y + layout.pattern.slider_pitch * i,
                   layout.slider.track_width, //FIXME
                   layout.slider.handle_y - layout.slider.name_y)){
                   //layout.slider.handle_width,
                   //layout.slider.handle_height)) {
            if(active_param_source)
                param_state_disconnect(active_param_source);
            if(active_param_source == &slots[index].param_states[i]){
                active_param_source = 0;
            }else{
                active_param_source = &slots[index].param_states[i];
            }
            return 1;
        }

    }

    // Else, drag the slot
    active_slot.index = index;
    active_slot.dx = 0;
    active_slot.dy = 0;

    mouse_drag_fn_p = &mouse_drag_slot;
    mouse_drop_fn_p = &mouse_drop_slot;
    return 1;

}

static int mouse_click_signal(int index, int x, int y)
{
    for(int i = 0; i < signals[index].n_params; i++)
    {
        // See if the click is on a parameter slider
        if(in_rect(x, y,
                   layout.signal.slider_start_x +
                   layout.slider.handle_start_x +
                     signals[index].param_states[i].value *
                     (layout.slider.track_width - layout.slider.handle_width),
                   layout.signal.slider_start_y +
                   layout.slider.handle_y + layout.signal.slider_pitch * i,
                   layout.slider.handle_width,
                   layout.slider.handle_height)) {
            if(signals[index].param_states[i].connected_output)
                return 1;
            active_param_slider.state = &signals[index].param_states[i];
            active_param_slider.initial_value = signals[index].param_states[i].value;
            mouse_drag_fn_p = &mouse_drag_param_slider;
            return 1;
        }
        // See if the click is on the parameter source button
        if(in_rect(x, y,
                   layout.signal.slider_start_x +
                   layout.slider.name_x,
                   layout.signal.slider_start_y +
                   layout.slider.name_y + layout.signal.slider_pitch * i,
                   layout.slider.track_width, //FIXME
                   layout.slider.handle_y - layout.slider.name_y)){
            if(active_param_source)
                param_state_disconnect(active_param_source);
            if(active_param_source == &signals[index].param_states[i]){
                active_param_source = 0;
            }else{
                active_param_source = &signals[index].param_states[i];
            }
            return 1;
        }

    }

    // Was the output clicked
    if(in_rect(x, y,
               layout.signal.output_x,
               layout.signal.output_y,
               layout.graph.width,
               layout.graph.height)){
        // Is there something looking for a source?
        if(active_param_source){
            param_state_connect(active_param_source, &signals[index].output);
            active_param_source = 0;
        }
    }

    return 0;
}

static int mouse_click_filter(int index, int x, int y){
    if(in_rect(x, y,
                layout.filter.waveform_x,
                layout.filter.waveform_y,
                layout.filter.waveform_width,
                layout.filter.waveform_height)){
        if(active_param_source){
            param_state_connect(active_param_source, &filters[index].output);
            active_param_source = 0;
            return 1;
        }
    }
    return 0;
}

static int mouse_click(int x, int y)
{
    // See if click is in a slot
    for(int i=0; i<n_slots; i++)
    {
        if(in_rect(x, y,
                   layout.slot_start_x + layout.slot_pitch * i,
                   layout.slot_start_y,
                   layout.slot_width, layout.slot_height))
        {
            // If it is, see if that slot wants to handle it
            return mouse_click_slot(i,
                                    x - (layout.slot_start_x + layout.slot_pitch * i),
                                    y - layout.slot_start_y);
        }
    }

    // See if click is in a pattern
    for(int i=0; i<n_slots; i++)
    {
        if(in_rect(x, y, 
                   layout.pattern_start_x + layout.pattern_pitch * i,
                   layout.pattern_start_y,
                   layout.pattern_width, layout.pattern_height))
        {
            active_pattern.index = i;
            active_pattern.dx = 0;
            active_pattern.dy = 0;
            mouse_drag_fn_p = &mouse_drag_pattern;
            mouse_drop_fn_p = &mouse_drop_pattern;
            return 1;
        }
    }

    // See if click is in an signal
    for(int i = 0; i < n_signals; i++){
        if(in_rect(x, y,
                   layout.signal.start_x + layout.signal.pitch * i,
                   layout.signal.start_y,
                   layout.signal.width,
                   layout.signal.height)){
            return mouse_click_signal(i,
                                     x - (layout.signal.start_x + layout.signal.pitch * i),
                                     y - layout.signal.start_y);
        }
    }

    // See if click is in filter
    for(int i = 0; i < n_filters; i++){
        if(in_rect(x, y,
                   layout.filter.start_x + layout.filter.pitch_x * i,
                   layout.filter.start_y + layout.filter.pitch_y * i,
                   layout.filter.width,
                   layout.filter.height)){

            return mouse_click_filter(i,
                    x - (layout.filter.start_x + layout.filter.pitch_x * i),
                    y - (layout.filter.start_y + layout.filter.pitch_y * i));
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
        }
    }
    return 1;
}

