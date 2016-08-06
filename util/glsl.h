#ifndef __UTIL_GLSL_H
#define __UTIL_GLSL_H

#include "util/common.h"

extern char * load_shader_error;

GLuint load_shader(const char * filename);

#endif
