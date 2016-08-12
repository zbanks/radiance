#pragma once
#include "util/common.h"
#include <semaphore.h>
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
    struct async_reader readback[2];
    int          prod_idx;
    int          cons_idx;
    int64_t      last_readback_completion;
    SDL_mutex * mutex;
    sem_t        semaphore;
};

void render_init(struct render * render, GLint texture);
void render_readback(struct render * render);
void render_term(struct render * render);

void render_wait(struct render *render);
void render_post(struct render *render);
void render_freeze(struct render * render);
void render_thaw(struct render * render);
SDL_Color render_sample(struct render * render, float x, float y);

#ifdef __cplusplus
}
#endif
