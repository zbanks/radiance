#include "deck.h"
#include "util/glsl.h"
#include "util/string.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define _STR2(x) #x
#define _STR(x)  _STR2(x)
#define DEBUG_INFO __FILE__ ":" _STR(__LINE__) ":" _STR(__func__)
#define ERROR(...) fprintf(stderr, "[error] [" DEBUG_INFO "] " __VA_ARGS__)
#define WARN(...)  fprintf(stderr, "[warn]  [" DEBUG_INFO "] " __VA_ARGS__)
#define INFO(...)  fprintf(stderr, "[info]  [" DEBUG_INFO "] " __VA_ARGS__)
#define DEBUG(...) fprintf(stderr, "[debug] [" DEBUG_INFO "] " __VA_ARGS__)
#define CONST(x) ((const typeof(x)) (x))


int deck_pattern_init(struct deck_pattern * pattern, const char * prefix) {
    memset(pattern, 0, sizeof *pattern);

    pattern->width = 100;
    pattern->height = 100;
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

        glGenFramebuffersEXT(1, &pattern->shaders[i].gl_fb);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pattern->shaders[i].gl_fb);
        glGenTextures(1, &pattern->shaders[i].gl_tex);
        glBindTexture(GL_TEXTURE_2D, pattern->shaders[i].gl_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pattern->width, pattern->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                pattern->shaders[i].gl_tex, 0);

        DEBUG("Loaded shader #%d", i);
    }

    return 0;
}

void deck_pattern_term(struct deck_pattern * pattern) {
    // Just warn for now; we'll have to implement this later? 
    if (pattern->replacing != NULL)
        ERROR("Term'ing pattern that is currently replacing %p!", pattern->replacing);

    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        struct deck_shader * shader = &pattern->shaders[i];
        //if (shader->gl_prog != 0)
        //    glDeleteProgramObjectARB(shader->gl_prog);
        if (shader->gl_fb != 0)
            glDeleteFramebuffers(1, &shader->gl_fb);
        if (shader->gl_tex != 0)
            glDeleteTextures(1, &shader->gl_tex);
    }

    memset(pattern, 0, sizeof *pattern);
}
