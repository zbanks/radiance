#ifndef __UTIL_GLSL_H
#define __UTIL_GLSL_H

#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif
extern char * load_shader_error;

bool   configure_vertex_area(float ww, float wh);
GLuint load_shader(const char * filename, bool is_ui);
#ifdef __cplusplus
}
#endif
#endif
