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
    glTextureStorage1D(tex, 1, GL_RGBA32F, length);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameterf(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    if(auto e = glGetError())
        FAIL("GL Error: %s\n",gluErrorString(e));
    return tex;
}
std::string read_file(const char * filename) {
    auto fn = std::string{filename};
    if(fn.size() && fn[0] == '#') {
        fn = config.paths.resource_path +std::string{"/"} +  fn.substr(1);
    }
    std::ifstream ifs(fn);
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}
std::string read_file(const std::string &filename) {
    return read_file(filename.c_str());
}

bool radGetShaderStatus(GLuint id)
{
    auto status = GLint{};
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    return status;
}
std::string radGetShaderInfoLog(GLuint id)
{
    auto length = GLint{};
    glGetShaderiv(id, GL_INFO_LOG_LENGTH,&length);
    auto result = std::string(length + 1,'\0');
    glGetShaderInfoLog(id, length, &length,&result[0]);
    result.resize(length);
    return result;

}
bool radGetProgramStatus(GLuint id)
{
    auto status = GLint{};
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    return status;
}
std::string radGetProgramInfoLog(GLuint id)
{
    auto length = GLint{};
    glGetProgramiv(id, GL_INFO_LOG_LENGTH,&length);
    auto result = std::string(length + 1,'\0');
    glGetProgramInfoLog(id, length, &length,&result[0]);
    result.resize(length);
    return result;

}
std::map<std::string, std::pair<GLuint, timespec> > s_shader_cache{};
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
                DEBUG("Using cached copy of shader \"%s\", %d\n", fn.c_str(),it->second.first);
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
    if(!radGetShaderStatus(shader)) {
        load_program_error = radGetShaderInfoLog(shader);
        WARN("Failed to compile shader \"%s\"\n %s\n", src.c_str(), load_program_error.c_str());
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}
GLuint load_program(
    std::initializer_list<const char *> filenames_vertex,
    std::initializer_list<const char *> filenames_geometry,
    std::initializer_list<const char *> filenames_noheader,
    const char *header,
    std::initializer_list<const char *> filenames_withheader)
{
    auto program = glCreateProgram();
    auto names_noheader = std::vector<GLuint>{};
    auto names_withheader = std::vector<GLuint>{};
    for(auto && name : filenames_vertex) {

        auto vshader = compile_shader(GL_VERTEX_SHADER, name);
        names_noheader.push_back(vshader);
        glAttachShader(program,vshader);
    }
    for(auto && name : filenames_geometry) {

        auto vshader = compile_shader(GL_GEOMETRY_SHADER, name);
        names_noheader.push_back(vshader);
        glAttachShader(program,vshader);
    }
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
    if(!radGetProgramStatus(program)) {
        load_program_error = radGetProgramInfoLog(program);
        glDeleteProgram(program);
        program = 0;
    }
    return program;
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

    if(!radGetProgramStatus(program)) {
        load_program_error = radGetProgramInfoLog(program);
        glDeleteProgram(program);
        glDetachShader(program, shader);
        program = 0;
    }
    glDeleteShader(shader);
    return program;
}
GLuint load_program(std::initializer_list<const char *> filenames_noheader,
                    const char *header,
                    std::initializer_list<const char *> filenames_withheader)
{
    return load_program({"#vertex_framed.glsl"},{"#geometry_framed.glsl"},filenames_noheader,header,filenames_withheader);
}
GLuint load_program(const char *vert,const char * frag)
{
    return load_program({vert},{},{"#lib.glsl"},"#header.glsl",{frag});
}
GLuint load_program_noheader(const char *vert,const char * frag)
{
    return load_program({vert},{},{frag},nullptr,{});
}
