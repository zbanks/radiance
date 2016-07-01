#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdbool.h>

#define MAX_INTEGRAL 1024

struct pattern {
    GLhandleARB * shader;
    int n_shaders;
    char * name;
    double intensity;
    double intensity_integral;

    int flip;
    GLuint * tex;
    GLuint fb;
    GLint * uni_tex;
    GLuint tex_output;
};

int pattern_init(struct pattern * pattern, const char * prefix);
void pattern_term(struct pattern * pattern);
void pattern_render(struct pattern * pattern, GLuint input_tex);
