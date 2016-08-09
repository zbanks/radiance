#ifndef __UTIL_GLSL_H
#define __UTIL_GLSL_H

#include "util/common.h"

std::string get_shader_error(void);

bool   configure_vertex_area(float ww, float wh);
GLuint load_pattern_shader(const char * filename);

GLuint load_one_shader(GLenum type, const char *filename);
GLuint make_shader_program(std::initializer_list<GLuint> shaders);
GLuint load_generic_program(const char * vert, const char *geom, const char *frag);
GLuint load_generic_program(const char * vert, const char *frag);
GLuint load_generic_program(const char *frag);
#endif
