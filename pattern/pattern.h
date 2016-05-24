#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <stdbool.h>

#define N_SHADERS_PER_PATTERN 3

struct pattern {
    GLhandleARB * shader;
    int n_shaders;
    char * name;
    double intensity;

    int flip;
    GLuint * tex;
    GLuint fb;
    GLint * uni_tex;
    GLuint tex_output;
};

int pattern_init(struct pattern * pattern, const char * prefix);
void pattern_term(struct pattern * pattern);
void pattern_render(struct pattern * pattern, GLuint input_tex);
