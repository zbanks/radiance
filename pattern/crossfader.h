#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdbool.h>
#include "pattern/deck.h"

struct crossfader {
    bool left_on_top;

    GLhandleARB shader;
    GLuint tex_output;
    GLuint fb;

    float position;
    uint8_t * rb_buf;
};

void crossfader_init(struct crossfader * crossfader);
void crossfader_term(struct crossfader * crossfader);
void crossfader_render(struct crossfader * crossfader, GLuint left, GLuint right);
