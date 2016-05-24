#pragma once

#include "pattern/pattern.h"

struct deck {
    struct pattern ** pattern; // e.g. deck.pattern[pattern_num]->intensity
    GLuint tex_input;
    GLuint fb_input;
    GLuint tex_output;
};

void deck_init(struct deck * deck);
void deck_term(struct deck * deck);
void deck_load_pattern(struct deck * deck, int slot, const char * prefix);
void deck_unload_pattern(struct deck * deck, int slot);
void deck_render(struct deck * deck);
