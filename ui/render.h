#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdint.h>
#include <SDL2/SDL.h>

struct render {
    GLuint fb;
    uint8_t * pixels;
    SDL_mutex * mutex;
};

void render_init(struct render * render, GLint texture);
void render_readback(struct render * render);
void render_term(struct render * render);

void render_freeze(struct render * render);
void render_thaw(struct render * render);
SDL_Color render_sample(struct render * render, float x, float y);
