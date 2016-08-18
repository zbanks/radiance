#include "util/common.h"
#include "util/glsl.h"
#include "util/err.h"
#include "util/config.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "util/string.h"
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>

namespace {
    std::string load_program_error{};
}
std::string get_load_program_error(void)
{
    return std::exchange(load_program_error,std::string{});
}

GLuint make_texture(int w, int h)
{
    return make_texture(GL_RGBA32F,w,h);
}
GLuint make_texture(GLenum format, int w, int h)
{
    auto tex = GLuint{};
    glGenTextures(1, &tex);
    if(!tex)
        return 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameterf(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameterf(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(tex, 1, format, w, h);
    if(auto e = glGetError())
        FAIL("GL Error: %s\n",gluErrorString(e));
    return tex;
}
GLuint make_texture(GLenum format, int w, int h, int layers)
{
    auto tex = GLuint{};
    glGenTextures(1, &tex);
    if(!tex)
        return 0;
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameterf(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameterf(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage3D(tex, 1, format, w, h, layers);
    if(auto e = glGetError())
        FAIL("GL Error: %s\n",gluErrorString(e));
    return tex;
}

GLuint make_texture(int length)
{
    auto tex = GLuint{};
    glGenTextures(1, &tex);
    if(!tex)
        return 0;
    glBindTexture(GL_TEXTURE_1D, tex);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameterf(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureStorage1D(tex, 1, GL_RGBA32F, length);
    if(auto e = glGetError())
        FAIL("GL Error: %s\n",gluErrorString(e));
    return tex;
}
static std::string read_file(const char * filename) {
    auto fn = std::string{filename};
    if(fn.size() && fn[0] == '#') {
        fn = config.paths.resource_path +std::string{"/"} +  fn.substr(1);
        INFO("Loading adjusted filename \"%s\"\n", fn.c_str());
    }
    std::ifstream ifs(fn);
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}
static std::string read_file(const std::string &filename) {
    return read_file(filename.c_str());
}

static bool getShaderStatus(GLuint id)
{
    auto status = GLint{};
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    return status;
}
static std::string getShaderInfoLog(GLuint id)
{
    auto length = GLint{};
    glGetShaderiv(id, GL_INFO_LOG_LENGTH,&length);
    auto result = std::string(length + 1,'\0');
    glGetShaderInfoLog(id, length, &length,&result[0]);
    result.resize(length);
    return result;

}
static bool getProgramStatus(GLuint id)
{
    auto status = GLint{};
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    return status;
}
static std::string getProgramInfoLog(GLuint id)
{
    auto length = GLint{};
    glGetProgramiv(id, GL_INFO_LOG_LENGTH,&length);
    auto result = std::string(length + 1,'\0');
    glGetProgramInfoLog(id, length, &length,&result[0]);
    result.resize(length);
    return result;

}
bool configure_vertex_area(float ww, float wh)
{
    glUniform2f(0, ww, wh);
    return true;
}
static std::map<std::string, std::pair<GLuint, timespec> > s_shader_cache{};
GLuint compile_shader(GLenum type, const std::string &filename)
{
    auto fn = filename;
    if(fn.size() && fn[0] == '#') {
        fn = "resources/" + fn.substr(1);
    }
    {
        auto it = s_shader_cache.find(fn);
        if(it != s_shader_cache.end()) {
            struct stat _stat{};
            ::stat(fn.c_str(), &_stat);
            if(it->second.second.tv_sec < _stat.st_mtim.tv_sec) {
                glDeleteShader(it->second.first);
                s_shader_cache.erase(it);
            }else{
                return it->second.first;
            }
        }
    }
    auto src = read_file(filename);
    if(src.empty()) {
        WARN("file unreadable or empty when compiling file %s\n", filename.c_str());
        return 0;
    }
    auto res = compile_shader_src(type, src);
    if(res) {
        struct stat _stat{};
        ::stat(fn.c_str(), &_stat);
        s_shader_cache.insert(std::make_pair(fn, std::make_pair(res, _stat.st_mtim)));
    }
    return res;
}
GLuint compile_shader_src(GLenum type, const std::string &src)
{
    auto shader = glCreateShader(type);
    if(!shader) {
        return 0;
    }
    const GLchar *srcp[] = { src.c_str()};
    const GLint   lenp[] = { GLint(src.size())};
    glShaderSource(shader, GLsizei{1}, srcp, lenp);
    glCompileShader(shader);
    if(!getShaderStatus(shader)) {
        load_program_error = getShaderInfoLog(shader);
        WARN("Failed to compile shader \"%s\"\n %s\n", src.c_str(), load_program_error.c_str());
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}
GLuint load_program(std::initializer_list<const char *> filenames_noheader,
                    const char *header,
                    std::initializer_list<const char *> filenames_withheader)
{
    auto program = glCreateProgram();
    auto vshader = compile_shader(GL_VERTEX_SHADER, "#vertex_framed.glsl");
    glAttachShader(program,vshader);
    auto gshader = compile_shader(GL_GEOMETRY_SHADER, "#geometry_framed.glsl");
    glAttachShader(program,gshader);
    auto names_noheader = std::vector<GLuint>{vshader,gshader};
    auto names_withheader = std::vector<GLuint>{};
    for(auto name : filenames_noheader) {
        auto tshader = compile_shader(GL_FRAGMENT_SHADER, name);
        if(!tshader){
            WARN("Failed to compile shader \"%s\"\n %s\n", name, load_program_error.c_str());
        }
        names_noheader.push_back(tshader);
        glAttachShader(program,tshader);
    }
    if(header) {
        auto head_buffer = read_file(header);
        for(auto name : filenames_withheader) {
            auto sn = std::string{name};
            auto prog_buffer = read_file(sn.c_str());
            auto tshader = compile_shader_src(GL_FRAGMENT_SHADER, head_buffer + prog_buffer);
            if(!tshader){
                WARN("Failed to compile shader \"%s\"\n %s\n", sn.c_str(), load_program_error.c_str());
            }
            names_withheader.push_back(tshader);
            glAttachShader(program,tshader);
        }

    }
    glLinkProgram(program);
    for(auto && name : names_noheader) {
        glDetachShader(program,name);
    }
    for(auto && name : names_withheader) {
        glDetachShader(program,name);
        glDeleteShader(name);
    }
    if(!getProgramStatus(program)) {
        load_program_error = getProgramInfoLog(program);
        glDeleteProgram(program);
        program = 0;
    }
    return program;
}

GLuint load_program(const char * filename)
{
    auto vshader = compile_shader(GL_VERTEX_SHADER, "#vertex_framed.glsl");
    auto gshader = compile_shader(GL_GEOMETRY_SHADER, "#geometry_framed.glsl");
    auto lshader = compile_shader(GL_FRAGMENT_SHADER, "#lib.glsl");
    auto head_buffer = read_file("#header.glsl");
    auto prog_buffer = read_file(filename);

    auto fshader = compile_shader_src(GL_FRAGMENT_SHADER, head_buffer + "\n" + prog_buffer);
    if(fshader) {
        auto program = glCreateProgram();
    //        glDeleteShader(vshader);
        // Link
        //glAttachShader(programObj, vertexShaderObj);
        glAttachShader(program, vshader);
        glAttachShader(program, gshader);
        glAttachShader(program, lshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        if(!getProgramStatus(program)) {
            load_program_error = getProgramInfoLog(program);


            glDeleteProgram(program);
            program = 0;
        }
        glDetachShader(program, vshader);
        glDetachShader(program, lshader);
        glDetachShader(program, gshader);
        glDetachShader(program, fshader);
        return program;
    }else{
        return 0;
    }
//    glDeleteShader(vshader);
//    glDeleteShader(gshader);
    glDeleteShader(fshader);

}
GLuint load_compute(const char *comp)
{

    // Load file
    auto shader = compile_shader(GL_COMPUTE_SHADER, std::string{comp});
    if(!shader)
        return 0;
    // Link
    auto program = glCreateProgram();
    //glAttachShader(programObj, vertexShaderObj);
    glAttachShader(program, shader);
    glLinkProgram(program);

    if(!getProgramStatus(program)) {
        load_program_error = getProgramInfoLog(program);
        glDeleteProgram(program);
        glDetachShader(program, shader);
        program = 0;
    }
    glDeleteShader(shader);
    return program;
}
GLuint load_program(const char *vert,const char * frag)
{

    // Load file
    auto vshader = compile_shader(GL_VERTEX_SHADER, std::string{vert});
    auto lshader = compile_shader(GL_FRAGMENT_SHADER, "#lib.glsl");
    auto head_buffer = read_file("#header.glsl");
    auto prog_buffer = read_file(frag);

    auto fshader = compile_shader_src(GL_FRAGMENT_SHADER, head_buffer + "\n" + prog_buffer);
    if(!fshader) {
        return 0;
    }
    // Link
    auto program = glCreateProgram();
    //glAttachShader(programObj, vertexShaderObj);
    glAttachShader(program, vshader);
    glAttachShader(program, lshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);
    glDetachShader(program, vshader);
    glDetachShader(program, lshader);
    glDetachShader(program, fshader);

    if(!getProgramStatus(program)) {
        load_program_error = getProgramInfoLog(program);
        glDeleteProgram(program);
//        glDetachShader(program, vshader);

        program = 0;
    }
    glDeleteShader(fshader);
//    glDeleteShader(vshader);
    return program;
}
GLuint load_program_noheader(const char *vert,const char * frag)
{

    // Load file
    auto vshader = compile_shader(GL_VERTEX_SHADER, std::string{vert});
    auto fshader = compile_shader(GL_FRAGMENT_SHADER,std::string(frag));
    if(!fshader) {
        glDeleteShader(vshader);
        return 0;
    }
    // Link
    auto program = glCreateProgram();
    //glAttachShader(programObj, vertexShaderObj);
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    if(!getProgramStatus(program)) {
        load_program_error = getProgramInfoLog(program);
        glDeleteProgram(program);
        glDetachShader(program, vshader);
        glDetachShader(program, fshader);

        program = 0;
    }
//    glDeleteShader(fshader);
//    glDeleteShader(vshader);
    return program;
}
