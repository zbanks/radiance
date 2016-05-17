#include "deck.h"
#include "util/glsl.h"
#include "util/string.h"
#include "util/err.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


static const char * const uniform_channel_names[N_LAYERS_PER_PATTERN] = {
    "iChannel0", "iChannel1", "iChannel2", "iChannel3"
};
static const char * const uniform_resolution_name = "iResolution";
static const char * const uniform_time_name = "iTime";

int deck_pattern_init(struct deck_pattern * pattern, const char * prefix) {
    memset(pattern, 0, sizeof *pattern);

    pattern->intensity = 1;

    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        memset(&pattern->shaders[i], 0, sizeof pattern->shaders[i]);

        char * filename = rsprintf("%s.%d.glsl", prefix, i);
        struct stat statbuf;
        int rc = stat(filename, &statbuf);

        if (rc != 0 || S_ISDIR(statbuf.st_mode)) {
            WARN("Unable to find file '%s' (%s)", filename, strerror(errno));
            free(filename);
            continue;
        }

        pattern->shaders[i].gl_prog = load_shader(filename);
        free(filename);

        if (pattern->shaders[i].gl_prog == 0) {
            if (i == 0) {
                ERROR("Unable to load primary pattern: %s", load_shader_error);
                return -1;
            } else {
                WARN("Unable to load pattern: %s", load_shader_error);
                continue;
            }
        }

        pattern->shaders[i].width = 100;
        pattern->shaders[i].height = 100;

        glGenTextures(2, pattern->shaders[i].gl_texs);

        for (int j = 0; j < 2; j++) {
            assert(pattern->shaders[i].gl_texs[j] != 0);

            glBindTexture(GL_TEXTURE_2D, pattern->shaders[i].gl_texs[j]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                    pattern->shaders[i].width, pattern->shaders[i].height, 0, 
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        }

        glBindTexture(GL_TEXTURE_2D, 0);

        DEBUG("Loaded shader #%d", i);
    }

    glGenFramebuffersEXT(1, &pattern->gl_fb);

    GLenum e;
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
    return 0;
}

void deck_pattern_term(struct deck_pattern * pattern) {
    // Just warn for now; we'll have to implement this later? 
    if (pattern->replacing != NULL)
        ERROR("Term'ing pattern that is currently replacing %p!", pattern->replacing);

    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        struct deck_shader * shader = &pattern->shaders[i];
        if (shader->gl_prog != 0) {
            //glDeleteProgramObjectARB(shader->gl_prog);
            glDeleteTextures(2, shader->gl_texs);
        }
    }

    glDeleteFramebuffers(1, &pattern->gl_fb);

    memset(pattern, 0, sizeof *pattern);
}

GLuint deck_pattern_render(struct deck_pattern * pattern, float time, GLuint base_tex) {
    GLenum e;

    // Reset The View
    glGetError();
    glPushMatrix();
    glLoadIdentity();

    size_t src_idx = pattern->bidx ? 0 : 1;
    size_t dst_idx = pattern->bidx ? 1 : 0;

    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        if(pattern->shaders[i].gl_prog == 0)
            continue;

        glActiveTexture(GL_TEXTURE0 + i);
        if (i == 0)
            glBindTexture(GL_TEXTURE_2D, pattern->shaders[i].gl_texs[src_idx]);
        else
            glBindTexture(GL_TEXTURE_2D, base_tex);
    }

    for (int z = N_LAYERS_PER_PATTERN - 1; z >= 0; z--) {
        if(pattern->shaders[z].gl_prog == 0) continue;

        glUseProgramObjectARB(pattern->shaders[z].gl_prog);

        GLint loc = glGetUniformLocationARB(pattern->shaders[z].gl_prog, uniform_resolution_name);
        glUniform2fARB(loc, pattern->shaders[z].width, pattern->shaders[z].height);

        loc = glGetUniformLocationARB(pattern->shaders[z].gl_prog, uniform_time_name);
        glUniform1fARB(loc, time);

        for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
            if(pattern->shaders[i].gl_texs[src_idx] == 0)
                continue;
            loc = glGetUniformLocationARB(pattern->shaders[z].gl_prog, uniform_channel_names[i]);
            glUniform1iARB(loc, i);
        }

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pattern->gl_fb);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                pattern->shaders[z].gl_texs[dst_idx], 0);

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);                // start drawing a polygon (4 sided)
        // XXX: Why doesn't this work with positive values?!
        glVertex2f(-1, -1);
        glVertex2f(0, -1);
        glVertex2f(0, 0);
        glVertex2f(-1, 0);
        glEnd();                    // done with the polygon

        if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

        if (z != 0) {
            glActiveTexture(GL_TEXTURE0 + z);
            glBindTexture(GL_TEXTURE_2D, pattern->shaders[z].gl_texs[dst_idx]);
        }
    }
    glPopMatrix();

    pattern->bidx = !pattern->bidx;

    return pattern->shaders[0].gl_texs[dst_idx];
}
