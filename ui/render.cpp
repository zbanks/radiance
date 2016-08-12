#include "ui/render.h"

#include "util/err.h"
#include "util/config.h"

#define BYTES_PER_PIXEL 4 // RGBA

void render_init(struct render * render, GLint texture)
{
    memset(render, 0, sizeof *render);
    sem_init(&render->semaphore, 0, 0);
//    render->pixels = static_cast<uint8_t*>(::calloc(config.pattern.master_width * config.pattern.master_height * BYTES_PER_PIXEL, sizeof(uint8_t)));
//    if(render->pixels == NULL) MEMFAIL();

    glGenFramebuffers(1, &render->fb);
    CHECK_GL();
   
    for(auto & rb : render->readback){
        glGenBuffers(1, &rb.pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, rb.pbo);
        rb.width = config.pattern.master_width;
        rb.height= config.pattern.master_height;
        CHECK_GL();
        glBufferStorage(
            GL_PIXEL_PACK_BUFFER
          , rb.width * rb.height * 4
          , NULL
          , GL_MAP_READ_BIT//|GL_MAP_PERSISTENT_BIT
        );
        CHECK_GL();
/*        rb.pixels = static_cast<uint8_t*>(glMapBufferRange(
            GL_PIXEL_PACK_BUFFER
          , 0
          , rb.width * rb.height * 4
          , GL_MAP_READ_BIT//|GL_MAP_PERSISTENT_BIT
           ));*/
        CHECK_GL();
        rb.fence = 0;
    }
    render->prod_idx = 0;
    render->cons_idx = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, render->fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    CHECK_GL();

    render->mutex = SDL_CreateMutex();
    if(render->mutex == NULL) FAIL("Could not create mutex: %s\n", SDL_GetError());
}

void render_term(struct render * render)
{
//    free(render->pixels);
    glDeleteFramebuffers(1, &render->fb);
    sem_destroy(&render->semaphore);
    for(auto &rb : render->readback){
        glDeleteSync(rb.fence);
        rb.fence = 0;
        if(rb.pixels) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER,rb.pbo);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        glDeleteBuffers(1, &rb.pbo);
        rb.pbo = 0;
        rb.pixels = nullptr;
        rb.width  = 0;
        rb.height = 0;
    }
    SDL_DestroyMutex(render->mutex);
    memset(render, 0, sizeof *render);
}

void render_readback(struct render * render)
{
    if(SDL_LockMutex(render->mutex) == 0) {
        auto &rb = render->readback[render->prod_idx&1];
        if(!rb.fence) {
//            INFO("Starting another fetch, prod_idx is %d\n",render->prod_idx);
            glBindFramebuffer(GL_FRAMEBUFFER, render->fb);
            CHECK_GL();
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            CHECK_GL();
            glBindBuffer(GL_PIXEL_PACK_BUFFER, rb.pbo);
            CHECK_GL();
            glReadPixels(0, 0, rb.width,rb.height,GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            CHECK_GL();
            glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
            CHECK_GL();
            rb.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
          ++render->prod_idx;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            CHECK_GL();
            glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
            CHECK_GL();
            SDL_UnlockMutex(render->mutex);
            sem_post(&render->semaphore);
        }
    }
}

void render_freeze(struct render * render) { SDL_LockMutex(render->mutex); }
void render_thaw(struct render * render) { SDL_UnlockMutex(render->mutex); }

SDL_Color render_sample(struct render * render, float x, float y)
{
    int col = 0.5 * (x + 1) * config.pattern.master_width;
    int row = 0.5 * (-y + 1) * config.pattern.master_height;
    if(col < 0) col = 0;
    if(row < 0) row = 0;
    if(col >= config.pattern.master_width) col = config.pattern.master_width - 1;
    if(row >= config.pattern.master_height) row = config.pattern.master_height - 1;
    auto index = BYTES_PER_PIXEL * (row * config.pattern.master_height + col);

    // Use NEAREST interpolation for now
    SDL_Color c;
    c.r = render->pixels[index];
    c.g = render->pixels[index + 1];
    c.b = render->pixels[index + 2];
    c.a = render->pixels[index + 3];
    return c;
}
void render_wait(struct render *render)
{
    while(sem_wait(&render->semaphore) < 0 && errno == EINTR)
    {}
}
void render_post(struct render *render)
{
    sem_post(&render->semaphore);
}
