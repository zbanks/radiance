#ifndef __UI_H
#define __UI_H
#include "util/common.h"

#include "pattern/pattern.h"
#include "util/config.h"
#if defined(USE_FREETYPE_GL) && USE_FREETYPE_GL
#include "text/ftgl.hpp"
#warning "Using full ftgl"
#else
#include "text/embedded.hpp"
#warning "Using embedded ftgl"
#endif
#include "util/err.h"
#include "util/glsl.h"
#include "util/math.h"
#include "midi/midi.h"
#include "output/output.h"
#include "audio/analyze.h"
#include "ui/render.h"
#include "output/slice.h"


void ui_init();
void ui_run();
void ui_term();
GLuint ui_tex_array(void);
SDL_GLContext ui_create_context();
void          ui_make_context_current(SDL_GLContext);

#endif
