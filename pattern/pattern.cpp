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
#include <stdexcept>
#include <exception>
#include <system_error>

#define GL_CHECK_ERROR() \
do { \
if(auto e = glGetError()) \
    FAIL("OpenGL error: %s\n", gluErrorString(e)); \
}while(false);

static GLuint vao = 0;
static GLuint vbo = 0;
pattern::pattern(const char * prefix)
{
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
            x, y, w, h
        };
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

    intensity = 0;
    intensity_integral = 0;
    name = prefix;

    int n = 0;
    for(;;) {
        struct stat statbuf;
        auto filename = std::string{config.pattern.dir} + std::string{prefix} + "." + std::to_string(n) + ".glsl";
        int rc = stat(filename.c_str(), &statbuf);

        if (rc != 0 || S_ISDIR(statbuf.st_mode))
            break;
        n++;
    }
    if(n == 0) {
        ERROR("Could not find any shaders for %s", prefix);
        throw std::system_error(EINVAL,std::system_category(),"failed to load shader.");
    }
    shader.clear();
    shader.reserve(n);
    tex.resize(n + 1);

    auto success = true;
    for(auto i = 0; i < n; i++) {
        auto filename = std::string{config.pattern.dir} + std::string{prefix} + "." + std::to_string(i) + ".glsl";
        auto h = load_shader(filename.c_str());
        if(!h) {
            success = false;
        }
        if (h == 0) {
            fprintf(stderr, "%s", get_load_shader_error().c_str());
            WARN("Unable to load shader %s", filename.c_str());
            success = false;
        } else {
            glProgramUniform2f(h, 0,  config.pattern.master_width, config.pattern.master_height);
            shader.push_back(h);
            DEBUG("Loaded shader #%d", i);
        }
    }
    if(!success) {
        ERROR("Failed to load some shaders.");
        throw std::system_error(EINVAL,std::system_category(),"failed to load shader.");
    }
    GL_CHECK_ERROR();

    // Render targets
    glGenFramebuffers(1, &fb);
    GL_CHECK_ERROR();
    glGenTextures(tex.size(), &tex[0]);
    GL_CHECK_ERROR();
    
    for(auto & t : tex) {
        glBindTexture(GL_TEXTURE_2D, t);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_ERROR();
        glTexStorage2D(GL_TEXTURE_2D, 1,GL_RGBA32F, config.pattern.master_width, config.pattern.master_height);
        GL_CHECK_ERROR();
        glClearTexImage(t, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    GL_CHECK_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR();

    uni_tex.resize(shader.size());
    std::iota(uni_tex.begin(),uni_tex.end(),1);
    // Some OpenGL API garbage
}

pattern::~pattern()
{
    for(auto & shader : shader) {
        glDeleteProgram(shader);
        shader = 0;
    }
    shader.clear();

    glDeleteTextures(tex.size(), &tex[0]);
    GL_CHECK_ERROR();
    tex.clear();
    glDeleteFramebuffers(1, &fb);
    GL_CHECK_ERROR();
    fb = 0;
    GL_CHECK_ERROR();
}

void pattern::render(GLuint input_tex) {

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBindVertexArray(vao);
    GL_CHECK_ERROR();
    glViewport(0, 0, config.pattern.master_width, config.pattern.master_height);
    GL_CHECK_ERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR();

    intensity_integral = fmod(intensity_integral + intensity / config.ui.fps, MAX_INTEGRAL);

    for (auto i = int(shader.size()) - 1; i >= 0; --i){
        glUseProgram(shader[i]);
        GL_CHECK_ERROR();
        
        auto tex_bindings = std::vector<GLuint>{};
        // Don't worry about this part.
        for(int j = 0; j < int(tex.size()) - 1; j++) {
            tex_bindings.push_back(tex[(flip + j + (i < j)) % (tex.size())]);
        }
        glBindTextures(1, tex_bindings.size(), &tex_bindings[0]);
        GL_CHECK_ERROR();
        glFramebufferTexture2D(
            GL_FRAMEBUFFER
          , GL_COLOR_ATTACHMENT0
          , GL_TEXTURE_2D
          , tex[(flip + i + 1) % (tex.size())]
          , 0);

        GL_CHECK_ERROR();
        glUniform1f(1, time_master.beat_frac + time_master.beat_index);
        glUniform4f(2, audio_low, audio_mid, audio_hi, audio_level);
        glUniform2f(3, config.pattern.master_width, config.pattern.master_height);
        glUniform1f(4, intensity);
        glUniform1f(5, intensity_integral);
        glUniform1f(6, config.ui.fps);
        glUniform1i(7, 0);
        glUniform1iv(8, uni_tex.size(), &uni_tex[0]);

        glActiveTexture(GL_TEXTURE0);
        GL_CHECK_ERROR();
        glBindTexture(GL_TEXTURE_2D, input_tex);
        GL_CHECK_ERROR();

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, 1);

        GL_CHECK_ERROR();
    }
    flip = (flip + 1) % (tex.size());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERROR();
    tex_output = tex[flip];
}
