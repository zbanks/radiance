#pragma once
#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct async_reader{
    GLuint    pbo;
    int       width;
    int       height;
    uint8_t  *pixels;
    GLsync    fence;
};
struct render {
    GLuint fb;
    uint8_t     *pixels; 
    async_reader readback[2];
    int          prod_idx;
    int          cons_idx;
    SDL_mutex * mutex;
};

void render_init(struct render * render, GLint texture);
void render_readback(struct render * render);
void render_term(struct render * render);

void render_freeze(struct render * render);
void render_thaw(struct render * render);
SDL_Color render_sample(struct render * render, float x, float y);

#ifdef __cplusplus
}
#endif
