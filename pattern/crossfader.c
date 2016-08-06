#include "util/common.h"
#include "pattern/crossfader.h"
#include "util/glsl.h"
#include "util/string.h"
#include "util/err.h"
#include "util/config.h"
#include <string.h>

void crossfader_init(struct crossfader * crossfader) {
    GLenum e;

    memset(crossfader, 0, sizeof *crossfader);

    crossfader->position = 0.5;

    crossfader->shader = load_shader("resources/crossfader.glsl");
    if(crossfader->shader == 0) FAIL("Unable to load crossfader shader:\n%s", load_shader_error);

    // Render targets
    glGenFramebuffers(1, &crossfader->fb);
    glGenTextures(1, &crossfader->tex_output);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindTexture(GL_TEXTURE_2D, crossfader->tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.pattern.master_width, config.pattern.master_height, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindFramebuffer(GL_FRAMEBUFFER, crossfader->fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                              crossfader->tex_output, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void crossfader_term(struct crossfader * crossfader) {
    GLenum e;

    glDeleteTextures(1, &crossfader->tex_output);
    glDeleteFramebuffers(1, &crossfader->fb);
    glDeleteProgram(crossfader->shader);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    memset(crossfader, 0, sizeof *crossfader);
}

void crossfader_render(struct crossfader * crossfader, GLuint left, GLuint right) {
    GLenum e;
    glLoadIdentity();
    glViewport(0, 0, config.pattern.master_width, config.pattern.master_height);
    glBindFramebuffer(GL_FRAMEBUFFER, crossfader->fb);
    glUseProgram(crossfader->shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, left);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, right);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    GLint loc;
    loc = glGetUniformLocation(crossfader->shader, "iResolution");
    glUniform2f(loc, config.pattern.master_width, config.pattern.master_height);
    loc = glGetUniformLocation(crossfader->shader, "iIntensity");
    glUniform1f(loc, crossfader->position);
    loc = glGetUniformLocation(crossfader->shader, "iFrameLeft");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(crossfader->shader, "iFrameRight");
    glUniform1i(loc, 1);
    loc = glGetUniformLocation(crossfader->shader, "iLeftOnTop");
    glUniform1i(loc, crossfader->left_on_top);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glVertex2d(-1, -1);
    glVertex2d(-1, 1);
    glVertex2d(1, 1);
    glVertex2d(1, -1);
    glEnd();

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    if(crossfader->position == 1.) {
        crossfader->left_on_top = true;
    } else if(crossfader->position == 0.) {
        crossfader->left_on_top = false;
    }
}
