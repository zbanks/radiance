#ifndef __UTIL_GLSL_H
#define __UTIL_GLSL_H

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

extern char * load_shader_error;

GLhandleARB load_shader(const char * filename);

#endif
