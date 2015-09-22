#ifndef __COLOR_H__
#define __COLOR_H__

#include <SDL/SDL.h>

typedef struct color
{
    float r;
    float g;
    float b;
    float a;
} color_t;

static inline uint32_t map_sdl_color(const SDL_Surface * surface, SDL_Color color){
    return SDL_MapRGB(surface->format, color.r, color.g, color.b);
}

uint32_t color_to_MapRGB(const SDL_PixelFormat * format, color_t color);
SDL_Color color_to_SDL(color_t color);

// --- Colormaps ---

struct colormap_el {
    float x;
    struct color y;
    float gamma;
};

#define COLORMAP_RESOLUTION 1025

enum colormap_state {
    COLORMAP_STATE_UNINITIALIZED = 0,
    COLORMAP_STATE_INVALID,
    COLORMAP_STATE_SAMPLED
};

struct colormap {
    const char name[32];
    size_t n_points;
    enum colormap_state state;
    color_t samples[COLORMAP_RESOLUTION];
    struct colormap_el points[];
};

int colormap_test_all();
color_t colormap_color(struct colormap * cm, float value);
int colormap_init(struct colormap * cm);
void colormap_set_global(struct colormap * cm);
void colormap_set_mono(float value);

extern int n_colormaps;
extern struct colormap * colormaps[];
extern struct colormap cm_rainbow;
extern struct colormap cm_rainbow_edged;
extern struct colormap cm_rainbow_equal;
extern struct colormap cm_jet;
extern struct colormap cm_hot;
extern struct colormap cm_cyan;
extern struct colormap cm_purple;
extern struct colormap cm_stoplight;
extern struct colormap cm_bright;
extern struct colormap cm_goth;

extern struct colormap * cm_global;
extern struct colormap * cm_global_mono;

#endif
