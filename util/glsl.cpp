#include "util/common.h"
#include "util/glsl.h"
#include "util/err.h"

#include "util/string.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {
    std::string _shader_error{};
}
std::string get_shader_error(void)
{
    return std::exchange(_shader_error,std::string{});
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
GLuint load_one_shader(GLenum type, const char *filename)
{
    auto fn = std::string(filename);
    if(fn[0] == '#')
        fn = "resources/" + fn.substr(1);

    auto buffer = read_file(filename);
    if(buffer.empty())
        return 0;
    auto shader = glCreateShader(type);
    const GLchar * sources[] = { buffer.c_str() };
    const GLint   lengths[] = { static_cast<GLint>(buffer.size()) };
    glShaderSource(shader, GLsizei{1}, sources, lengths);
    glCompileShader(shader);
    if(!getShaderStatus(shader)) {
        _shader_error = getShaderInfoLog(shader);
        glDeleteShader(shader);
        shader = 0;
        return false;
    }
    return shader;
}
GLuint make_shader_program(std::initializer_list<GLuint> shaders)
{
    auto program = glCreateProgram();
    if(!program)
        return program;
    for(auto && shader : shaders)
        glAttachShader(program,shader);
    glLinkProgram(program);
    if(!getProgramStatus(program)) {
        _shader_error = getProgramInfoLog(program);
        glDeleteProgram(program);
        program = 0;
    }else{
        for(auto && shader : shaders)
            glDetachShader(program,shader);
    }
    return program;
}
GLuint load_generic_program(const char * filename)
{
    auto vshader = load_one_shader(GL_VERTEX_SHADER, "#gvertex.glsl");
    if(!vshader)
        return 0;
    auto gshader = load_one_shader(GL_GEOMETRY_SHADER,"#ggeometry.glsl");
    if(!gshader){
        glDeleteShader(vshader);
        return 0;
    }
    auto fshader = load_one_shader(GL_FRAGMENT_SHADER,filename);
    if(!fshader) {
        glDeleteShader(vshader);
        glDeleteShader(gshader);
        return 0;
    }
    auto prog = make_shader_program({vshader,gshader,fshader});
    glDeleteShader(vshader);
    glDeleteShader(gshader);
    glDeleteShader(fshader);
    return prog;
}
GLuint load_generic_program(const char *vert, const char * frag)
{
    auto vshader = load_one_shader(GL_VERTEX_SHADER, vert);
    if(!vshader)
        return 0;
    auto fshader = load_one_shader(GL_FRAGMENT_SHADER,frag);
    if(!fshader) {
        glDeleteShader(vshader);
        return 0;
    }
    auto prog = make_shader_program({vshader,fshader});
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    return prog;
}
GLuint load_generic_program(const char *vert, const char *geom, const char *frag)
{
    auto vshader = load_one_shader(GL_VERTEX_SHADER, vert);
    if(!vshader)
        return 0;
    auto gshader = load_one_shader(GL_GEOMETRY_SHADER,geom);
    if(!gshader){
        glDeleteShader(vshader);
        return 0;
    }
    auto fshader = load_one_shader(GL_FRAGMENT_SHADER,frag);
    if(!fshader) {
        glDeleteShader(vshader);
        glDeleteShader(gshader);
        return 0;
    }
    auto prog = make_shader_program({vshader,gshader,fshader});
    glDeleteShader(vshader);
    glDeleteShader(gshader);
    glDeleteShader(fshader);
    return prog;
}
GLuint load_pattern_shader(const char * filename)
{
    if(!vertexShader && !(vertexShader = load_one_shader(GL_VERTEX_SHADER, "#vertex_framed.glsl"))){
        return 0;
    }
    if(!geometryShader && !(geometryShader = load_one_shader(GL_GEOMETRY_SHADER, "#geometry_framed.glsl"))) {
        return 0;
    }
    // Load file
    auto prog_buffer = read_file(filename);
    auto head_buffer = read_file("resources/header.glsl");
    if(prog_buffer.empty() || head_buffer.empty())
        return 0;
    head_buffer = "#version 430\n" + head_buffer;
    // Compile
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    auto merged = head_buffer + prog_buffer;
    
    const GLchar * sources[] = { merged.c_str() };
    const GLint   lengths[] = { static_cast<GLint>(merged.size()) };
    glShaderSource(fragmentShader, GLsizei{1}, sources, lengths);
    glCompileShader(fragmentShader);
    if(!getShaderStatus(fragmentShader)) {
        _shader_error = getShaderInfoLog(fragmentShader);
        glDeleteShader(fragmentShader);
        return 0;
    }
    
    auto program = make_shader_program({vertexShader,geometryShader,fragmentShader});
    glDeleteShader(fragmentShader);
    return program;
}
