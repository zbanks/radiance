#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <stdbool.h>

#define N_LAYERS_PER_PATTERN 3
#define SCRATCHPAD_WIDTH 64
#define SCRATCHPAD_HEIGHT 64

struct render_target {
    int width;
    int height;
    double transform[9];
    GLuint tex_screen[N_LAYERS_PER_PATTERN];
    GLuint tex_scratchpad;
    GLuint tex_scratchpad_out;
    GLuint tex_screen_out;

    GLuint fb_screen;
    GLuint fb_scratchpad;
    GLuint fb_screen_dest;
    GLuint fb_scratchpad_dest;
};

struct pattern {
    GLhandleARB shader_layer[N_LAYERS_PER_PATTERN];
    GLhandleARB shader_scratchpad;
    char * name;
    double intensity;
    struct render_target ** rt; // Array of render target pointers
    int n_rt; // Length of rt
};

void pattern_render_target_init(struct render_target * render_target, int width, int height, const double * transform);
void pattern_render_target_term(struct render_target * render_target);

int pattern_init(struct pattern * pattern, const char * prefix, struct render_target ** render_targets, int n_render_targets);
void pattern_term(struct pattern * pattern);
void pattern_render(struct pattern * pattern, double time, GLuint * input_tex);

