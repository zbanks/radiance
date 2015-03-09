#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "err.h"
#include "frame.h"
#include <stdint.h>
#include <math.h>

static SDL_Window* win;
static SDL_Surface* screen;
static SDL_Surface* master_preview;
static SDL_Surface* pattern_preview;
static SDL_Surface* slot_pane;
static TTF_Font* font;

int ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        ERROR("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init())
    {
        ERROR("TTF_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    win = SDL_CreateWindow("Hello World!", 100, 100, 1024, 600, SDL_WINDOW_SHOWN);
    if (!win){
        ERROR("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }

    screen = SDL_GetWindowSurface(win);

    if (!screen)
    {
        SDL_DestroyWindow(win);
        ERROR("SDL_GetWindowSurface Error: %s\n", SDL_GetError());
        return 1;
    }

    master_preview = SDL_CreateRGBSurface(0, 200, 200, 32, 0, 0, 0, 0);

    if(!master_preview)
    {
        ERROR("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
        return 1;
    }

    pattern_preview = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);

    if(!pattern_preview)
    {
        ERROR("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
        return 1;
    }

   slot_pane = SDL_CreateRGBSurface(0, 110, 200, 32, 0, 0, 0, 0);

    if(!slot_pane)
    {
        ERROR("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
        return 1;
    }

    font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 10);

    if(!font)
    {
        ERROR("TTF_OpenFont Error: %s\n", SDL_GetError());
        return 1;
    }

    return 0;
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

    for(int x=0;x<master_preview->w;x++)
    {
        for(int y=0;y<master_preview->h;y++)
        {
            float xf = ((float)x / (master_preview->w - 1)) * 2 - 1;
            float yf = ((float)y / (master_preview->h - 1)) * 2 - 1;
            color_t pixel = render_composite(xf, yf);
            ((uint32_t*)(master_preview->pixels))[x + master_preview->w * y] = SDL_MapRGB(
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

    for(int x=0;x<pattern_preview->w;x++)
    {
        for(int y=0;y<pattern_preview->h;y++)
        {
            float xf = ((float)x / (pattern_preview->w - 1)) * 2 - 1;
            float yf = ((float)y / (pattern_preview->h - 1)) * 2 - 1;
            color_t pixel = (*slot->pattern->render)(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + pattern_preview->w * y] = SDL_MapRGB(
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
    r.w = slot_pane->w;
    r.h = slot_pane->h;
    SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 20, 20, 20));

    if(slot->pattern)
    {
        update_pattern_preview(slot);
        r.w = pattern_preview->w;
        r.h = pattern_preview->h;
        r.x = 5;
        r.y = 5;
        SDL_BlitSurface(pattern_preview, 0, slot_pane, &r);

        SDL_Color white = {255, 255, 255};

        for(int i = 0; i < slot->pattern->n_params; i++)
        {
            SDL_Surface* msg = TTF_RenderText_Solid(font, slot->pattern->parameters[i].name, white);
            r.x = 3;
            r.y = 110+15*i;
            r.w = msg->w;
            r.h = msg->h;
            SDL_BlitSurface(msg, 0, slot_pane, &r);
            SDL_FreeSurface(msg);
        }

    }
    else
    {
        r.x = 1;
        r.y = 1;
        r.w = slot_pane->w-2;
        r.h = slot_pane->h-2;
        SDL_FillRect(slot_pane, &r, SDL_MapRGB(pattern_preview->format, 0, 0, 0));
    }
}

void ui_render()
{
    SDL_Rect r;

    update_master_preview();
    r.x = 30;
    r.y = 30;
    r.w = master_preview->w;
    r.h = master_preview->h;
    SDL_BlitSurface(master_preview, 0, screen, &r);

    for(int i=0; i<n_slots; i++)
    {
        ui_update_slot(&slots[i]);
        r.w = slot_pane->w;
        r.h = slot_pane->h;
        r.x = 10+120*i;
        r.y = 250;
        SDL_BlitSurface(slot_pane, 0, screen, &r);
    }

    SDL_UpdateWindowSurface(win);
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
        }
    }
    return 0;
}

