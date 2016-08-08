#pragma once
#include "util/common.h"


#include <audio/audio.h>

NOT_CXX
void analyze_init();
void analyze_chunk(chunk_pt chunk);
void analyze_render(GLuint tex_spectrum, GLuint tex_waveform, GLuint tex_waveform_beats);
void analyze_term();
CXX_OK
