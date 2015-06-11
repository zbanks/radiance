#include <math.h>

#include <SDL/SDL.h>

#include "util/color.h"
#include "util/math.h"

uint32_t color_to_MapRGB(const SDL_PixelFormat * format, color_t color){
    return SDL_MapRGB(format, 
                      (uint8_t) roundf(255 * color.r),
                      (uint8_t) roundf(255 * color.g),
                      (uint8_t) roundf(255 * color.b));
}

SDL_Color color_to_SDL(color_t color){
    return (SDL_Color) {(uint8_t) roundf(255 * color.r),
                        (uint8_t) roundf(255 * color.g),
                        (uint8_t) roundf(255 * color.b),
                        (uint8_t) roundf(255 * color.a)};
}

// --- Colormaps ---

// This colormap is special, it fades from black/*color*/white
// where *color* is taken from the global colormap
struct colormap s_cm_global_mono = {
    .name = "Global Mono",
    .n_points = 3,
    .state = COLORMAP_STATE_UNINITIALIZED,
    .points = {
        { .x = 0.0,
          .y = {0.0, 0.0, 0.0, 1.},
          .gamma = 1.5,
        },
        { .x = 0.5,
          .y = {0.5, 0.5, 0.5, 1.},
          .gamma = 1./1.5,
        },
        { .x = 1.0,
          .y = {1.0, 1.0, 1.0, 1.},
          .gamma = 1.,
        },
    },
};

struct colormap * cm_global = &cm_rainbow;
struct colormap * cm_global_mono = &s_cm_global_mono;

int n_colormaps = 9;
struct colormap * colormaps[9] =  {&s_cm_global_mono, &cm_rainbow, &cm_rainbow_edged, &cm_rainbow_equal, &cm_jet, &cm_hot, &cm_cyan, &cm_purple, &cm_stoplight};

static float mono_value = 0;

color_t colormap_color(struct colormap * cm, float value){
    //if(colormap_test(cm)) return (color_t) {0,0,0,1};
    if(cm->state == COLORMAP_STATE_UNINITIALIZED)
        colormap_init(cm);
    if(cm->state != COLORMAP_STATE_SAMPLED)
        return (color_t) {0,0,0, 1.0};

    value = MIN(MAX(value, 0.), 1.);
    value *= (COLORMAP_RESOLUTION - 1);
    int idx = (int) value;
#ifdef COLORMAP_EXACT
    float alpha = fmod(value,  1.0);
    color_t left = cm->samples[idx];
    if(alpha < 1e-4)
        return left;
    color_t right = cm->samples[idx+1];
    color_t out;
    out.r = right.r * alpha + left.r * (1.0 - alpha);
    out.g = right.g * alpha + left.g * (1.0 - alpha);
    out.b = right.b * alpha + left.b * (1.0 - alpha);
    out.a = 1;
    return out;
#else 
    return cm->samples[idx];
#endif
}

static int colormap_test(struct colormap * cm){
    // Test to make sure the colormap is valid:
    // 1. There are at least 2 points
    // 2. The first point has x = 0
    // 3. The last point has x = 1
    // 4. The x's are monotonically increasing
    // 5. All of the color channels are on [0, 1]
    // 6. Each gamma is positive
    if(!cm) return -1;
    if(cm->n_points < 2) return -1;
    if(cm->points[0].x != 0) return -1;
    if(cm->points[cm->n_points-1].x != 1) return -1;
    float last_x = 0;
    for(size_t i = 0; i < cm->n_points; i++){
        if(cm->points[i].x < last_x) return -1;
        if((cm->points[i].y.r < 0) || (cm->points[i].y.r > 1)) return -1;
        if((cm->points[i].y.g < 0) || (cm->points[i].y.g > 1)) return -1;
        if((cm->points[i].y.b < 0) || (cm->points[i].y.b > 1)) return -1;
        if(cm->points[i].gamma <= 0) return -1;
        last_x = cm->points[i].x;
    }
    return 0;
}


int colormap_test_all(){
    int rc = 0;
    for(int i = 0; i < n_colormaps; i++){
        if(colormap_test(colormaps[i])){
            printf("Error in colormap '%s'\n", colormaps[i]->name);
            rc = -1;
        }
    }
    return rc;
}

int colormap_init(struct colormap * cm){
    if(cm->state != COLORMAP_STATE_UNINITIALIZED) return 0;
    if(colormap_test(cm)){
        cm->state = COLORMAP_STATE_INVALID;
        printf("Error in colormap '%s'\n", cm->name);
        return -1;
    }

    for(size_t i = 0; i < COLORMAP_RESOLUTION; i++){
        float value = (float) i / (COLORMAP_RESOLUTION - 1.);
        struct colormap_el * left = cm->points;
        struct colormap_el * right = left+1;

        while(right->x < value){
            left++;
            right++;
        }

        float t = powf((value - left->x) / (right->x - left->x), left->gamma);

        color_t out;
        out.r = right->y.r * t + left->y.r * (1.0 - t);
        out.g = right->y.g * t + left->y.g * (1.0 - t);
        out.b = right->y.b * t + left->y.b * (1.0 - t);
        //out.a = right->y.a * t + left->y.a * (1.0 - t);
        out.a = 1;
        cm->samples[i] = out;
    }
    cm->state = COLORMAP_STATE_SAMPLED;
    return 0;
}

void colormap_set_global(struct colormap * cm){
    cm_global = cm;
    colormap_set_mono(mono_value);
}

void colormap_set_mono(float value){
    mono_value = value;
    s_cm_global_mono.points[1].y = colormap_color(cm_global, mono_value);
    s_cm_global_mono.state = COLORMAP_STATE_UNINITIALIZED;
    colormap_init(&s_cm_global_mono);

}
