#include "util/common.h"
#include "pattern/crossfader.h"
#include "util/glsl.h"
#include "util/string.h"
#include "util/err.h"
#include "util/config.h"
#include <string.h>
#include "ui/ui.h"
static GLuint vao = 0;
static GLuint vbo = 0;

void crossfader_init(struct crossfader * crossfader) {
    bool new_buffers = false;
    if(!vao) {
        glGenVertexArrays(1,&vao);
        new_buffers = true;
    }
    if(!vbo)  {
        glGenBuffers(1,&vbo);
        new_buffers = true;
        float w = config.pattern.master_width,h =  config.pattern.master_height;
        float x = 0.f, y = 0.f;
        GLfloat vertices[] = {
            x, y, w, h
        };
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    }
    if(new_buffers){
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindVertexArray(vao);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
    }

    memset(crossfader, 0, sizeof *crossfader);

    crossfader->position = 0.5;

    crossfader->shader = load_shader("resources/crossfader.glsl");
    if(crossfader->shader == 0) FAIL("Unable to load crossfader shader:\n%s", get_load_shader_error().c_str());
    auto h = crossfader->shader;
    glUseProgram(h);
    glProgramUniform2f(h,0,  config.pattern.master_width, config.pattern.master_height);
    CHECK_GL();
    auto loc = crossfader->loc.iResolution = glGetUniformLocation(crossfader->shader, "iResolution");
    glProgramUniform2f(h,loc, config.pattern.master_width, config.pattern.master_height);
    CHECK_GL();
    loc = crossfader->loc.iIntensity  = glGetUniformLocation(crossfader->shader, "iIntensity");
    glProgramUniform1f(h,loc, crossfader->position);
    CHECK_GL();
    loc = crossfader->loc.iFrameLeft = glGetUniformLocation(crossfader->shader, "iFrameLeft");
    glProgramUniform1i(h,loc, 0);
    CHECK_GL();
    loc = crossfader->loc.iFrameRight = glGetUniformLocation(crossfader->shader, "iFrameRight");
    glProgramUniform1i(h,loc, 1);
    CHECK_GL();
    loc = crossfader->loc.iLeftOnTop = glGetUniformLocation(crossfader->shader, "iLeftOnTop");
    glProgramUniform1i(h, loc, crossfader->left_on_top);
    CHECK_GL();

    // Render targets
    glGenFramebuffers(1, &crossfader->fb);
    glGenTextures(1, &crossfader->tex_output);
    CHECK_GL();

    glBindTexture(GL_TEXTURE_2D, crossfader->tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, config.pattern.master_width, config.pattern.master_height);
    CHECK_GL();

    glBindFramebuffer(GL_FRAMEBUFFER, crossfader->fb);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,crossfader->tex_output, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL();
}

void crossfader_term(struct crossfader * crossfader) {
    glDeleteTextures(1, &crossfader->tex_output);
    glDeleteFramebuffers(1, &crossfader->fb);
    glDeleteProgram(crossfader->shader);
    CHECK_GL();

    memset(crossfader, 0, sizeof *crossfader);
}

void crossfader_render(struct crossfader * crossfader, GLuint left, GLuint right) {
    glViewport(0, 0, config.pattern.master_width, config.pattern.master_height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, crossfader->fb);
    glUseProgram(crossfader->shader);
    CHECK_GL();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    CHECK_GL();
    GLuint texs[] = { left, right};
    glBindTextures(0, 2, texs);
    CHECK_GL();
    auto &loc = crossfader->loc;
    glProgramUniform1f(crossfader->fb,loc.iIntensity, crossfader->position);
    glProgramUniform1i(crossfader->fb, loc.iLeftOnTop, crossfader->left_on_top);
    CHECK_GL();

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_POINTS, 0, 1);
    CHECK_GL();

    if(crossfader->position == 1.) {
        crossfader->left_on_top = true;
    } else if(crossfader->position == 0.) {
        crossfader->left_on_top = false;
    }
}
