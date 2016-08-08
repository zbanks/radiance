#include "util/common.h"
#include "ui/ui.h"
#include "pattern/pattern.h"
#include "time/timebase.h"
#include "util/glsl.h"
#include "util/string.h"
#include "sys/stat.h"
#include "util/err.h"
#include "util/config.h"
#include "main.h"

static GLuint vao = 0;
static GLuint vbo = 0;
int pattern_init(struct pattern * pattern, const char * prefix) {
    bool new_buffers = false;
    if(!vao) {
        glGenVertexArrays(1,&vao);
        new_buffers = true;
    }
    if(!vbo)  {
        glGenBuffers(1,&vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        new_buffers = true;
        float w = config.pattern.master_width,h =  config.pattern.master_height;
        float x = 0.f, y = 0.f;
        GLfloat vertices[] = {
            x, y, w, h, x + 0, y + 0,
            x, y, w, h, x + 0, y + h,
            x, y, w, h, x + w, y + 0,
            x, y, w, h, x + w, y + h
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    }
    if(new_buffers){
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindVertexArray(vao);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4*sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
    }
    GLenum e;

    memset(pattern, 0, sizeof *pattern);

    pattern->intensity = 0;
    pattern->intensity_integral = 0;
    pattern->name = strdup(prefix);
    if(pattern->name == NULL) ERROR("Could not allocate memory");

    int n = 0;
    for(;;) {
        char * filename;
        struct stat statbuf;
        filename = rsprintf("%s%s.%d.glsl", config.pattern.dir, prefix, n);
        if(filename == NULL) MEMFAIL();

        int rc = stat(filename, &statbuf);
        free(filename);

        if (rc != 0 || S_ISDIR(statbuf.st_mode)) {
            break;
        }
        n++;
    }

    if(n == 0) {
        ERROR("Could not find any shaders for %s", prefix);
        return 1;
    }
    pattern->n_shaders = n;

    pattern->shader = calloc(pattern->n_shaders, sizeof *pattern->shader);
    if(pattern->shader == NULL) MEMFAIL();
    pattern->tex = calloc(pattern->n_shaders, sizeof *pattern->tex);
    if(pattern->tex == NULL) MEMFAIL();

    bool success = true;
    for(int i = 0; i < pattern->n_shaders; i++) {
        char * filename;
        filename = rsprintf("%s%s.%d.glsl", config.pattern.dir, prefix, i);
        if(filename == NULL) MEMFAIL();
        GLuint h = load_shader(filename,false);
        glProgramUniform2f(h, 0,  config.pattern.master_width, config.pattern.master_height);
        if (h == 0) {
            fprintf(stderr, "%s", load_shader_error);
            WARN("Unable to load shader %s", filename);
            success = false;
        } else {
            pattern->shader[i] = h;
            DEBUG("Loaded shader #%d", i);
        }
        free(filename);
    }
    if(!success) {
        ERROR("Failed to load some shaders.");
        return 2;
    }

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    // Render targets
    glGenFramebuffers(1, &pattern->fb);
    glGenTextures(pattern->n_shaders + 1, pattern->tex);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    for(int i = 0; i < pattern->n_shaders + 1; i++) {
        glBindTexture(GL_TEXTURE_2D, pattern->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexStorage2D(GL_TEXTURE_2D, 1,GL_RGBA32F, config.pattern.master_width, config.pattern.master_height);
    }

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindFramebuffer(GL_FRAMEBUFFER, pattern->fb);
    for(int i = 0; i < pattern->n_shaders + 1; i++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                  pattern->tex[i], 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    // Some OpenGL API garbage
    pattern->uni_tex = calloc(pattern->n_shaders, sizeof *pattern->uni_tex);
    if(pattern->uni_tex == NULL) MEMFAIL();
    for(int i = 0; i < pattern->n_shaders; i++) {
        pattern->uni_tex[i] = i + 1;
    }

    return 0;
}

void pattern_term(struct pattern * pattern) {
    GLenum e;

    for (int i = 0; i < pattern->n_shaders; i++) {
        glDeleteProgram(pattern->shader[i]);
    }

    glDeleteTextures(pattern->n_shaders + 1, pattern->tex);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
    glDeleteFramebuffers(1, &pattern->fb);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    free(pattern->name);
    memset(pattern, 0, sizeof *pattern);
}

void pattern_render(struct pattern * pattern, GLuint input_tex) {
    GLenum e;
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBindVertexArray(vao);
    glViewport(0, 0, config.pattern.master_width, config.pattern.master_height);
    glBindFramebuffer(GL_FRAMEBUFFER, pattern->fb);

    pattern->intensity_integral = fmod(pattern->intensity_integral + pattern->intensity / config.ui.fps, MAX_INTEGRAL);

    for (int i = pattern->n_shaders - 1; i >= 0; i--) {
        glUseProgram(pattern->shader[i]);

        // Don't worry about this part.
        for(int j = 0; j < pattern->n_shaders; j++) {
            // Or, worry about it, but don't think about it.
            glActiveTexture(GL_TEXTURE1 + j);
            glBindTexture(GL_TEXTURE_2D, pattern->tex[(pattern->flip + j + (i < j)) % (pattern->n_shaders + 1)]);
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                  pattern->tex[(pattern->flip + i + 1) % (pattern->n_shaders + 1)], 0);

        if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

        glUniform1f(1, time_master.beat_frac + time_master.beat_index);
        glUniform4f(2, audio_low, audio_mid, audio_hi, audio_level);
        glUniform2f(3, config.pattern.master_width, config.pattern.master_height);
        glUniform1f(4, pattern->intensity);
        glUniform1f(5, pattern->intensity_integral);
        glUniform1f(6, config.ui.fps);
        glUniform1i(7, 0);
        glUniform1iv(8, pattern->n_shaders, pattern->uni_tex);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input_tex);

        if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
    }
    pattern->flip = (pattern->flip + 1) % (pattern->n_shaders + 1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
    pattern->tex_output = pattern->tex[pattern->flip];
}
