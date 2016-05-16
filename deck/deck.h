#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <stdbool.h>

#define N_PATTERNS_PER_DECK  4
#define N_LAYERS_PER_PATTERN 4

struct deck_shader {
    GLhandleARB gl_prog;
    GLuint gl_fbs[2];
    GLuint gl_texs[2];
    float width;
    float height;
};

struct deck_pattern {
    struct deck_pattern * replacing;
    float intensity;
    bool bidx;
    struct deck_shader shaders[N_LAYERS_PER_PATTERN];
};

struct deck {
    struct deck_pattern * deck_patterns[N_PATTERNS_PER_DECK];
};

int deck_pattern_init(struct deck_pattern * pattern, const char * prefix);
void deck_pattern_term(struct deck_pattern * pattern);
GLuint deck_pattern_render(struct deck_pattern * pattern, float time, GLuint base_tex);
