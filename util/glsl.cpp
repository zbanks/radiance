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
/*
const char default_vertex_shader[] = "                          \n\
#version 130                                                    \n\
void main(void) {                                               \n\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;     \n\
}                                                               \n\
";
*/

static std::string read_file(const char * filename) {
    std::ifstream ifs(filename);
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
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
GLuint load_shader(const char * filename, bool is_ui)
{
    if(!vertexShader) {
        auto vert_buffer = read_file("resources/vertex.glsl");
        if(vert_buffer.empty())
            return 0;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar * sources[] = { vert_buffer.c_str() };
        const GLint   lengths[] = { static_cast<GLint>(vert_buffer.size()) };
        glShaderSource(vertexShader, GLsizei{1}, sources, lengths);
        glCompileShader(vertexShader);
        if(!getShaderStatus(vertexShader)) {
            load_shader_error = getShaderInfoLog(vertexShader);
            glDeleteShader(vertexShader);
            vertexShader = 0;
            return false;
        }
    }
    if(!geometryShader) {
        auto vert_buffer = read_file("resources/geometry.glsl");
        if(vert_buffer.empty())
            return 0;
        geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        const GLchar * sources[] = { vert_buffer.c_str() };
        const GLint   lengths[] = { static_cast<GLint>(vert_buffer.size()) };
        glShaderSource(geometryShader, GLsizei{1}, sources, lengths);
        glCompileShader(geometryShader);
        if(!getShaderStatus(geometryShader)) {
            load_shader_error = getShaderInfoLog(geometryShader);
            glDeleteShader(geometryShader);
            vertexShader = 0;
            return false;
        }
    }
    // Load file
    auto prog_buffer = read_file(filename);

    auto head_buffer = read_file("resources/header.glsl");

    if(is_ui)
        head_buffer = "#version 430\n#define RENDERING_UI 1\n" + head_buffer;
    else
        head_buffer = "#version 430\n" + head_buffer;
    if(prog_buffer.empty() || head_buffer.empty())
        return 0;
    // Compile
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    auto merged = head_buffer + prog_buffer;
    
    const GLchar * sources[] = { merged.c_str() };
    const GLint   lengths[] = { static_cast<GLint>(merged.size()) };
    glShaderSource(fragmentShader, GLsizei{1}, sources, lengths);
    glCompileShader(fragmentShader);
    if(!getShaderStatus(fragmentShader)) {
        load_shader_error = getShaderInfoLog(fragmentShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

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
