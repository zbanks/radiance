#include <SDL2/SDL.h>
#include "err.h"
#include "frame.h"
#include <stdint.h>
#include <math.h>

static SDL_Window* win;
static SDL_Surface* screen;
static SDL_Surface* master;
static SDL_Surface* pattern;

int ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        ERROR("SDL_Init Error: %s\n", SDL_GetError());
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

    master = SDL_CreateRGBSurface(0, 200, 200, 32, 0, 0, 0, 0);

    if(!master)
    {
        ERROR("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
        return 1;
    }

    pattern = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);

    if(!pattern)
    {
        ERROR("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
        return 1;
    }


    return 0;
}

void ui_quit()
{
    SDL_FreeSurface(master);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

static void update_master()
{
    SDL_LockSurface(master);

    for(int x=0;x<master->w;x++)
    {
        for(int y=0;y<master->h;y++)
        {
            float xf = ((float)x / (master->w - 1)) * 2 - 1;
            float yf = ((float)y / (master->h - 1)) * 2 - 1;
            color_t pixel = render_composite(xf, yf);
            ((uint32_t*)(master->pixels))[x + master->w * y] = SDL_MapRGB(
                master->format,
                (uint8_t)roundf(255*pixel.r),
                (uint8_t)roundf(255*pixel.g),
                (uint8_t)roundf(255*pixel.b));
        }
    }

    SDL_UnlockSurface(master);
}

static void update_pattern(pat_render_fn_pt render, pat_state_pt state_p)
{
    if(render)
    {
        SDL_LockSurface(pattern);

        for(int x=0;x<pattern->w;x++)
        {
            for(int y=0;y<pattern->h;y++)
            {
                float xf = ((float)x / (pattern->w - 1)) * 2 - 1;
                float yf = ((float)y / (pattern->h - 1)) * 2 - 1;
                color_t pixel = (*render)(xf, yf, state_p);
                ((uint32_t*)(pattern->pixels))[x + pattern->w * y] = SDL_MapRGB(
                    pattern->format,
                    (uint8_t)roundf(255 * pixel.r * pixel.a),
                    (uint8_t)roundf(255 * pixel.g * pixel.a),
                    (uint8_t)roundf(255 * pixel.b * pixel.a));
            }
        }

        SDL_UnlockSurface(pattern);
    }
    else
    {
        // No pattern
        SDL_Rect r;
        r.x = 0;
        r.y = 0;
        r.w = pattern->w;
        r.h = pattern->h;
        SDL_FillRect(pattern, &r, SDL_MapRGB(pattern->format, 20, 20, 20));
    }
}

void ui_render()
{
    SDL_Rect r;

    update_master();
    r.x = 30;
    r.y = 30;
    r.w = master->w;
    r.h = master->h;
    SDL_BlitSurface(master, 0, screen, &r);

    r.w = pattern->w;
    r.h = pattern->h;
    for(int i=0; i<n_slots; i++)
    {
        update_pattern(pat_render_fns[i], pat_states[i]);
        r.x = 30+120*i;
        r.y = 250;
        SDL_BlitSurface(pattern, 0, screen, &r);
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

