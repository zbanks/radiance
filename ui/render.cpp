#include "ui/render.h"

#include "output/slice.h"
#include "util/err.h"
#include "util/glsl.h"
#include "util/config.h"

#define BYTES_PER_PIXEL 16 // RGBA

void render_init(struct render * render, GLint texture)
{
    auto texel_count = config.pattern.master_width*config.pattern.master_height;
//    render->prog = load_compute("#render.c.glsl");
//    render->pixels = static_cast<GLfloat*>(::calloc(texel_count, BYTES_PER_PIXEL));//::calloc(config.pattern.master_width * config.pattern.master_height, BYTES_PER_PIXEL));
//    if(render->pixels == NULL)
//        MEMFAIL();

    glGenBuffers(1, &render->pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, render->pbo);
    glBufferStorage(
        GL_PIXEL_PACK_BUFFER
      , texel_count * BYTES_PER_PIXEL
      , nullptr,
        GL_MAP_READ_BIT
       |GL_MAP_PERSISTENT_BIT
       |GL_MAP_COHERENT_BIT
       );
    render->pixels = static_cast<GLfloat*>(
        glMapBufferRange(
            GL_PIXEL_PACK_BUFFER
          , 0
          , texel_count * BYTES_PER_PIXEL
          , GL_MAP_READ_BIT
           |GL_MAP_PERSISTENT_BIT
           |GL_MAP_COHERENT_BIT
           )
        );
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glGenFramebuffers(1, &render->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, render->fb);
    glNamedFramebufferTexture(render->fb, GL_COLOR_ATTACHMENT0, texture, 0);
    CHECK_GL();

    render->mutex = SDL_CreateMutex();
    if(render->mutex == NULL)
        FAIL("Could not create mutex: %s\n", SDL_GetError());
}

void render_term(struct render * render) {
//    free(render->pixels);
    glBindBuffer(GL_PIXEL_PACK_BUFFER,render->pbo);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glDeleteBuffers(1,&render->pbo);
    glDeleteFramebuffers(1, &render->fb);
    SDL_DestroyMutex(render->mutex);
    memset(render, 0, sizeof *render);
}

void render_readback(struct render * render)
{
    if(SDL_TryLockMutex(render->mutex) == 0) {
        if(render->fence) {
            glDeleteSync(render->fence);
            render->fence = nullptr;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, render->fb);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, render->pbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glMemoryBarrier(
            GL_PIXEL_BUFFER_BARRIER_BIT
            |GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
        );
        glReadPixels(0, 0, config.pattern.master_width, config.pattern.master_height, GL_RGBA, GL_FLOAT, NULL);//(GLvoid*)render->pixels);
        render->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        CHECK_GL();
        SDL_UnlockMutex(render->mutex);
    }else{
        WARN("failed to initiate render readback: output thread is lagging.");
    }

}

bool render_freeze(struct render * render) {
    SDL_LockMutex(render->mutex);
    auto fence = render->fence;
    SDL_UnlockMutex(render->mutex);
    if(fence) {
        auto signaled = false;
        auto success  = true;
        while(!signaled) {
            switch(glClientWaitSync(fence, 0, 1000000)) {
                case GL_TIMEOUT_EXPIRED:
                    continue;
                case GL_WAIT_FAILED:{
                    ERROR("Failed to wait on GLsync object.\n");
                    success = false;
                }
                case GL_ALREADY_SIGNALED:
                case GL_CONDITION_SATISFIED:
                    DEBUG("Exiting sync loop.");
                    signaled = true;;
            }
        }
        return success;
    }else{
        WARN("No sync available.");
    }
    return false;
}

void render_thaw(struct render * render)
{
    SDL_LockMutex(render->mutex);
    if(render->fence) {
        glDeleteSync(render->fence);
        render->fence = nullptr;
    }else{
        WARN("Thae with no prior fence!.");
    }
    SDL_UnlockMutex(render->mutex);
}

SDL_Color render_sample(struct render * render, float x, float y)
{
    int col = 0.5 * (x + 1) * config.pattern.master_width;
    int row = 0.5 * (-y + 1) * config.pattern.master_height;
    if(col < 0) col = 0;
    if(row < 0) row = 0;
    if(col >= config.pattern.master_width) col = config.pattern.master_width - 1;
    if(row >= config.pattern.master_height) row = config.pattern.master_height - 1;
    long index = BYTES_PER_PIXEL * (row * config.pattern.master_height + col);

    // Use NEAREST interpolation for now
    SDL_Color c;
    c.r = render->pixels[index]*255;
    c.g = render->pixels[index + 1]*255;
    c.b = render->pixels[index + 2]*255;
    c.a = render->pixels[index + 3]*255;
    return c;
}
