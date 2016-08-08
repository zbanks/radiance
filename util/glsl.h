#ifndef __UTIL_GLSL_H
#define __UTIL_GLSL_H

#include "util/common.h"

std::string get_load_shader_error(void);

bool   configure_vertex_area(float ww, float wh);
GLuint load_shader(const char * filename, bool is_ui);

#endif
