#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "err.h"
#include "slot.h"
#include <stdint.h>
#include <math.h>

static SDL_Window* win;
static SDL_Surface* screen;
static SDL_Surface* master_preview;
static SDL_Surface* pattern_preview;
static SDL_Surface* slot_pane;
static TTF_Font* font;

static int mouse_down;
static int mouse_drag_start_x;
static int mouse_drag_start_y;

static const struct
{
    int master_x;
    int master_y;
    int master_width;
    int master_height;

    int slot_start_x;
    int slot_start_y;
    int slot_width;
    int slot_height;
    int slot_pitch;

    int pattern_x;
    int pattern_y;
    int pattern_width;
    int pattern_height;

    int param_text_start_x;
    int param_text_start_y;
    int param_pitch;
    int param_slider_start_x;
    int param_slider_start_y;
    int param_slider_width;
    int param_handle_start_x;
    int param_handle_start_y;
    int param_handle_width;
    int param_handle_height;
} layout = {
    .master_x = 30,
    .master_y = 30,
    .master_width = 200,
    .master_height = 200,

    .slot_start_x = 10,
    .slot_start_y = 250,
    .slot_width = 110,
    .slot_height = 250,
    .slot_pitch = 125,

    .pattern_x = 5,
    .pattern_y = 5,
    .pattern_width = 100,
    .pattern_height = 100,

    .param_text_start_x = 3,
    .param_text_start_y = 110,
    .param_pitch = 30,
    .param_slider_start_x = 3,
    .param_slider_start_y = 125,
    .param_slider_width = 70,
    .param_handle_start_x = 3,
    .param_handle_start_y = 120,
    .param_handle_width = 10,
    .param_handle_height = 10,
};

static void (*mouse_drag_fn_p)(int x, int y);
static void (*mouse_drop_fn_p)();

static struct
{
    slot_t* slot;
    int index;
    float initial_value;
} active_param_slider;

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

    win = SDL_CreateWindow("Hello World!", 100, 100, 1024, 600, SDL_WINDOW_SHOWN);
    if (!win) FAIL("SDL_CreateWindow Error: %s\n", SDL_GetError());

    screen = SDL_GetWindowSurface(win);

    if (!screen) FAIL("SDL_GetWindowSurface Error: %s\n", SDL_GetError());

    master_preview = SDL_CreateRGBSurface(0, layout.master_width, layout.master_height, 32, 0, 0, 0, 0);

    if(!master_preview) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    pattern_preview = SDL_CreateRGBSurface(0, layout.pattern_width, layout.pattern_height, 32, 0, 0, 0, 0);

    if(!pattern_preview) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

   slot_pane = SDL_CreateRGBSurface(0, layout.slot_width, layout.slot_height, 32, 0, 0, 0, 0);

    if(!slot_pane) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());

    font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 10);

    if(!font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    mouse_down = 0;
    mouse_drag_fn_p = 0;
    mouse_drop_fn_p = 0;
}

void ui_quit()
{
    SDL_FreeSurface(master_preview);
    SDL_FreeSurface(pattern_preview);
    SDL_FreeSurface(slot_pane);
    TTF_CloseFont(font);
    SDL_DestroyWindow(win);
    SDL_Quit();
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
                (uint8_t)roundf(255*pixel.r),
                (uint8_t)roundf(255*pixel.g),
                (uint8_t)roundf(255*pixel.b));
        }
    }

    SDL_UnlockSurface(master_preview);
}

static void update_pattern_preview(slot_t* slot)
{
    SDL_LockSurface(pattern_preview);

    for(int x = 0; x < layout.pattern_width; x++)
    {
        for(int y=0 ;y < layout.pattern_height; y++)
        {
            float xf = ((float)x / (layout.pattern_width - 1)) * 2 - 1;
            float yf = ((float)y / (layout.pattern_height - 1)) * 2 - 1;
            color_t pixel = (*slot->pattern->render)(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.pattern_width * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * pixel.r * pixel.a),
                (uint8_t)roundf(255 * pixel.g * pixel.a),
                (uint8_t)roundf(255 * pixel.b * pixel.a));
        }
    }

    SDL_UnlockSurface(pattern_preview);
}

void ui_update_slot(slot_t* slot)
{
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = layout.slot_width;
    r.h = layout.slot_height;
    SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 20, 20, 20));

    if(slot->pattern)
    {
        update_pattern_preview(slot);
        r.w = layout.pattern_width;
        r.h = layout.pattern_height;
        r.x = layout.pattern_x;
        r.y = layout.pattern_y;
        SDL_BlitSurface(pattern_preview, 0, slot_pane, &r);

        SDL_Color white = {255, 255, 255};

        for(int i = 0; i < slot->pattern->n_params; i++)
        {
            SDL_Surface* msg = TTF_RenderText_Solid(font, slot->pattern->parameters[i].name, white);
            r.x = layout.param_text_start_x;
            r.y = layout.param_text_start_y+layout.param_pitch*i;
            r.w = msg->w;
            r.h = msg->h;
            SDL_BlitSurface(msg, 0, slot_pane, &r);
            SDL_FreeSurface(msg);

            r.x = layout.param_slider_start_x;
            r.y = layout.param_slider_start_y + layout.param_pitch*i;
            r.w = layout.param_slider_width;
            r.h = 3;
            SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 80, 80, 80));

            r.x = layout.param_handle_start_x +
                  slot->param_values[i] * (layout.param_slider_width - layout.param_handle_width);
            r.y = layout.param_handle_start_y + layout.param_pitch * i;
            r.w = layout.param_handle_width;
            r.h = layout.param_handle_height;

            SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 0, 0, 80));
        }
    }
    else
    {
        r.x = 1;
        r.y = 1;
        r.w = layout.slot_width-2;
        r.h = layout.slot_height-2;
        SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 0, 0, 0));
    }
}

void ui_render()
{
    SDL_Rect r;

    update_master_preview();
    r.x = layout.master_x;
    r.y = layout.master_y;
    r.w = layout.master_width;
    r.h = layout.master_height;
    SDL_BlitSurface(master_preview, 0, screen, &r);

    for(int i=0; i<n_slots; i++)
    {
        ui_update_slot(&slots[i]);
        r.w = slot_pane->w;
        r.h = slot_pane->h;
        r.x = layout.slot_start_x+layout.slot_pitch*i;
        r.y = layout.slot_start_y;
        SDL_BlitSurface(slot_pane, 0, screen, &r);
    }

    SDL_UpdateWindowSurface(win);
}

static int in_rect(int x, int y, int rx, int ry, int rw, int rh)
{
    return x >= rx && y >= ry &&
           x < rx + rw && y < ry + rh;
}

static void mouse_drag_param_slider(int x, int y)
{
    float val = active_param_slider.initial_value +
                (float)x / (layout.param_slider_width - layout.param_handle_width);

    if(val < 0) val = 0;
    else if(val > 1) val = 1;

    active_param_slider.slot->param_values[active_param_slider.index] = val;
}

static int mouse_click_slot(slot_t* slot, int x, int y)
{
    // See if the click is on a parameter slider
    for(int i = 0; i < slot->pattern->n_params; i++)
    {
        if(in_rect(x, y,
                   layout.param_handle_start_x +
                     slot->param_values[i] *
                     (layout.param_slider_width - layout.param_handle_width),
                   layout.param_handle_start_y + layout.param_pitch * i,
                   layout.param_handle_width,
                   layout.param_handle_height))
        {
            active_param_slider.slot = slot;
            active_param_slider.index = i;
            active_param_slider.initial_value = slot->param_values[i];
            mouse_drag_fn_p = &mouse_drag_param_slider;
            return 1;
        }
    }

    return 0;
}

static int mouse_click_master(int x, int y)
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
            return mouse_click_slot(&slots[i],
                                    x - (layout.slot_start_x + layout.slot_pitch * i),
                                    y - layout.slot_start_y);
        }
    }

    // Otherwise, do not handle click
    return 0;
}

int ui_poll()
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_QUIT:
                return 1;
            case SDL_MOUSEBUTTONDOWN:
                mouse_click_master(e.button.x, e.button.y);
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
    return 0;
}

