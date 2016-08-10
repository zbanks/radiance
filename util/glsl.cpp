#include "util/common.h"
#include "util/glsl.h"
#include "util/err.h"

#include "util/string.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {
    std::string load_shader_error{};
}
std::string get_load_shader_error(void)
{
    return std::exchange(load_shader_error,std::string{});
}

GLuint make_texture(int w, int h)
{
    auto tex = GLuint{};
    glGenTextures(1, &tex);
    if(!tex)
        return 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, w, h);
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
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGBA32F, length);
    if(auto e = glGetError())
        FAIL("GL Error: %s\n",gluErrorString(e));
    return tex;
}


/*
const char default_vertex_shader[] = "                          \n\
#version 130                                                    \n\
void main(void) {                                               \n\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;     \n\
}                                                               \n\
";
*/
static std::string read_file(const char * filename) {
    auto fn = std::string{filename};
    if(fn.size() && fn[0] == '#') {
        fn = "resources/" + fn.substr(1);
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
static GLuint vertexShader = 0;
static GLuint geometryShader = 0;
bool configure_vertex_area(float ww, float wh)
{
    glUniform2f(0, ww, wh);
    return true;
}
GLuint compile_shader(GLenum type, const std::string &filename)
{
    auto src = read_file(filename);
    if(src.empty()) {
        WARN("file unreadable or empty when compiling file %s\n", filename.c_str());
        return 0;
    }
    return compile_shader_src(type, src);
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
        load_shader_error = getShaderInfoLog(shader);
        WARN("Failed to compile shader \"%s\"\n %s\n", src.c_str(), load_shader_error.c_str());
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}
GLuint load_shader(const char * filename)
{
    if(!vertexShader && !(vertexShader = compile_shader(GL_VERTEX_SHADER,std::string{"#vertex_framed.glsl"}))) {
        WARN("Failed to compile vertex shader\n");
        return 0;
    }
    if(!geometryShader && !(geometryShader = compile_shader(GL_GEOMETRY_SHADER,std::string{"#geometry_framed.glsl"}))) {
        WARN("Failed to compile geometry shader\n");
        return 0;
    }
    // Load file
    auto head_buffer = read_file("#header.glsl");
    auto prog_buffer = read_file(filename);

    if(prog_buffer.empty() || head_buffer.empty())
        return 0;
    auto fragmentShader = compile_shader_src(GL_FRAGMENT_SHADER, head_buffer + "\n" + prog_buffer);
    if(!fragmentShader)
        return 0;
    // Link
    auto program = glCreateProgram();
    //glAttachShader(programObj, vertexShaderObj);
    glAttachShader(program, vertexShader);
    glAttachShader(program, geometryShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    if(!getProgramStatus(program)) {
        load_shader_error = getProgramInfoLog(program);
        glDeleteProgram(program);
        program = 0;
    }
    glDetachShader(program, fragmentShader);
    glDetachShader(program, geometryShader);
    glDetachShader(program, vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}
GLuint load_shader(const char *vert,const char * frag)
{

    // Load file
    auto vshader = compile_shader(GL_VERTEX_SHADER, std::string{vert});
    auto head_buffer = read_file("#header.glsl");
    auto prog_buffer = read_file(frag);

    auto fshader = compile_shader_src(GL_FRAGMENT_SHADER, head_buffer + "\n" + prog_buffer);
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
        load_shader_error = getProgramInfoLog(program);
        glDeleteProgram(program);
        program = 0;
    }
    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    glDeleteShader(fshader);
    glDeleteShader(vshader);
    return program;
}
