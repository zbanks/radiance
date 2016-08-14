#ifndef __UTIL_GLSL_H
#define __UTIL_GLSL_H

#include "util/common.h"

std::string get_load_shader_error(void);

bool   configure_vertex_area(float ww, float wh);
GLuint compile_shader(GLenum type, const std::string &file);
GLuint compile_shader_src(GLenum type, const std::string &src);

GLuint load_shader(const char * filename);
GLuint load_shader(const char *vert, const char * frag);
GLuint load_shader_noheader(const char *vert, const char * frag);
GLuint load_compute(const char *filename);

GLuint make_texture(int w, int h);
GLuint make_texture(GLenum format, int w, int h);
GLuint make_texture(int length);
#endif
