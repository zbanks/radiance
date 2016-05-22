#include "deck/pattern.h"
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

static void setup_texture(GLuint tex, int width, int height) {
    assert(tex != 0);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void pattern_render_target_init(struct render_target * render_target, int width, int height, const double * transform) {
    GLenum e;

    memset(render_target, 0, sizeof *render_target);
    render_target->width = width;
    render_target->height = height;
    memcpy(render_target->transform, transform, 9 * sizeof(double));

    glGenFramebuffersEXT(1, &render_target->fb_screen);
    glGenFramebuffersEXT(1, &render_target->fb_scratchpad);
    glGenFramebuffersEXT(1, &render_target->fb_screen_dest);
    glGenFramebuffersEXT(1, &render_target->fb_scratchpad_dest);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glGenTextures(N_LAYERS_PER_PATTERN, render_target->tex_screen);
    glGenTextures(1, &render_target->tex_scratchpad);
    glGenTextures(1, &render_target->tex_screen_out);
    glGenTextures(1, &render_target->tex_scratchpad_out);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        setup_texture(render_target->tex_screen[i], width, height);
    }
    setup_texture(render_target->tex_scratchpad, SCRATCHPAD_WIDTH, SCRATCHPAD_HEIGHT);
    setup_texture(render_target->tex_screen_out, width, height);
    setup_texture(render_target->tex_scratchpad_out, SCRATCHPAD_WIDTH, SCRATCHPAD_HEIGHT);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, render_target->fb_screen);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                              render_target->tex_screen_out, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, render_target->fb_scratchpad);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                              render_target->tex_scratchpad_out, 0);

    // Unlike the layer destination FBO, the scratchpad destination FBO always points to the same texture
    // so set it up here.
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, render_target->fb_scratchpad_dest);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                              render_target->tex_scratchpad, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void pattern_render_target_term(struct render_target * render_target) {
    glDeleteTextures(N_LAYERS_PER_PATTERN, render_target->tex_screen);
    glDeleteTextures(1, &render_target->tex_scratchpad);
    glDeleteTextures(1, &render_target->tex_screen_out);
    glDeleteTextures(1, &render_target->tex_scratchpad_out);

    glDeleteFramebuffersEXT(1, &render_target->fb_screen);
    glDeleteFramebuffersEXT(1, &render_target->fb_scratchpad);

    memset(render_target, 0, sizeof *render_target);

    GLenum e;
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void clear_render_target(struct render_target * rt) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fb_scratchpad);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fb_screen);
    for(int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                                  rt->tex_screen[i], 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

GLhandleARB try_load_shader(char * filename) {
    struct stat statbuf;
    int rc = stat(filename, &statbuf);

    if (rc != 0 || S_ISDIR(statbuf.st_mode)) {
        WARN("Unable to find file '%s' (%s)", filename, strerror(errno));
        return 0;
    }

    GLhandleARB h = load_shader(filename);
    if (h == 0) {
        WARN("%s", load_shader_error);
    }
    return h;
}

int pattern_init(struct pattern * pattern, const char * prefix, struct render_target ** render_targets, int n_render_targets) {
    memset(pattern, 0, sizeof *pattern);

    pattern->intensity = 1;
    pattern->name = strdup("unnamed");
    if(pattern->name == NULL) ERROR("Could not allocate memory");
    pattern->rt = render_targets;
    pattern->n_rt = n_render_targets;

    for(int i = 0; i < pattern->n_rt; i++) {
        clear_render_target(pattern->rt[i]);
    }

    char * filename;
    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        filename = rsprintf("%s.%d.glsl", prefix, i);
        if(filename == NULL) ERROR("Could not allocate memory");
        pattern->shader_layer[i] = try_load_shader(filename);
        free(filename);

        if (pattern->shader_layer[i] == 0) {
            if (i == 0) {
                ERROR("Unable to load primary pattern");
                return -1;
            } else {
                WARN("Unable to load pattern #%d", i);
                continue;
            }
        }
        DEBUG("Loaded shader #%d", i);
    }
    filename = rsprintf("%s.scratchpad.glsl", prefix);
    if(filename == NULL) ERROR("Could not allocate memory");
    pattern->shader_scratchpad = try_load_shader(filename);
    free(filename);
    if(pattern->shader_scratchpad == 0) {
        WARN("Unable to load scratchpad");
    }

    GLenum e;
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
    return 0;
}

void pattern_term(struct pattern * pattern) {
    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        if (pattern->shader_layer[i] != 0) glDeleteObjectARB(pattern->shader_layer[i]);
    }
    if (pattern->shader_scratchpad != 0) glDeleteObjectARB(pattern->shader_scratchpad);

    memset(pattern, 0, sizeof *pattern);
}

static void render(GLhandleARB prog, double time, double intensity, int width, int height, double * transform) {
    GLint loc;

    loc = glGetUniformLocationARB(prog, "iTime");
    glUniform1fARB(loc, time);

    loc = glGetUniformLocationARB(prog, "iResolution");
    glUniform2fARB(loc, width, height);

    loc = glGetUniformLocationARB(prog, "iFrame");
    glUniform1iARB(loc, 0);

    loc = glGetUniformLocationARB(prog, "iIntensity");
    glUniform1fARB(loc, intensity);

    loc = glGetUniformLocationARB(prog, "iScratchpad");
    glUniform1iARB(loc, 1);

    const GLfloat mat[9] = {
        2 * transform[0] / width, transform[1], transform[2],
        transform[3], 2 * transform[4] / height, transform[5],
        -1 - 2 * transform[6] / width, -1 - 2 * transform[7] / height, transform[8]
    };

    loc = glGetUniformLocationARB(prog, "iTransform");
    glUniformMatrix3fvARB(loc, 1, false, mat);

    GLenum e;
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    char layer_uniform_name[8] = "iLayerX";
    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        layer_uniform_name[sizeof(layer_uniform_name) - 2] = '0' + i;
        loc = glGetUniformLocationARB(prog, layer_uniform_name);
        glUniform1iARB(loc, 2 + i);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glVertex2d(-1, -1);
    glVertex2d(-1, 1);
    glVertex2d(1, 1);
    glVertex2d(1, -1);
    glEnd();
}

static void bind_textures(struct render_target * render_target, GLuint input_tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, render_target->tex_scratchpad);

    for (int i = 0; i < N_LAYERS_PER_PATTERN; i++) {
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, render_target->tex_screen[i]);
    }
}

void pattern_render(struct pattern * pattern, double time, GLuint * input_tex) {
    GLenum e;

    glLoadIdentity();
    glViewport(0, 0, SCRATCHPAD_WIDTH, SCRATCHPAD_HEIGHT);

    if(pattern->shader_scratchpad != 0) {
        glUseProgramObjectARB(pattern->shader_scratchpad);
        for(int i = 0; i < pattern->n_rt; i++) {
            bind_textures(pattern->rt[i], input_tex[i]);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pattern->rt[i]->fb_scratchpad);
            render(pattern->shader_scratchpad, time, pattern->intensity, SCRATCHPAD_WIDTH, SCRATCHPAD_HEIGHT, pattern->rt[i]->transform);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pattern->rt[i]->fb_scratchpad_dest);
            glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, pattern->rt[i]->fb_scratchpad);
            glBlitFramebuffer(0, 0, SCRATCHPAD_WIDTH, SCRATCHPAD_HEIGHT,
                              0, 0, SCRATCHPAD_WIDTH, SCRATCHPAD_HEIGHT,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }

    for (int z = N_LAYERS_PER_PATTERN; z >= 0; z--) {
        if(pattern->shader_layer[z] == 0) continue;

        glUseProgramObjectARB(pattern->shader_layer[z]);

        for(int i = 0; i < pattern->n_rt; i++) {
            bind_textures(pattern->rt[i], input_tex[i]);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pattern->rt[i]->fb_screen);
            glViewport(0, 0, pattern->rt[i]->width, pattern->rt[i]->height);
            render(pattern->shader_layer[z], time, pattern->intensity, pattern->rt[i]->width, pattern->rt[i]->height, pattern->rt[i]->transform);

            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pattern->rt[i]->fb_screen_dest);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                                      pattern->rt[i]->tex_screen[z], 0);
            glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, pattern->rt[i]->fb_screen);
            glBlitFramebuffer(0, 0, pattern->rt[i]->width, pattern->rt[i]->height,
                              0, 0, pattern->rt[i]->width, pattern->rt[i]->height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
    }
}

