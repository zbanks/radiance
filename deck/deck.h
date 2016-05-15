#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

#define N_PATTERNS_PER_DECK  4
#define N_LAYERS_PER_PATTERN 4

struct deck_shader {
    GLhandleARB gl_prog;
    GLuint gl_fb;
    GLuint gl_tex;
};

struct deck_pattern {
    struct deck_pattern * replacing;
    float height;
    float width;
    float intensity;
    struct deck_shader shaders[N_LAYERS_PER_PATTERN];
};

struct deck {
    struct deck_pattern * deck_patterns[N_PATTERNS_PER_DECK];
};

