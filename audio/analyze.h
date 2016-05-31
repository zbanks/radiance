#pragma once

#include <audio/audio.h>
#include <SDL2/SDL_opengl.h>

void analyze_init();
void analyze_chunk(chunk_pt chunk);
void analyze_render(GLuint tex_spectrum, GLuint tex_waveform);
void analyze_term();
