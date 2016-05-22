#ifndef __DECK_H
#define __DECK_H

#include "deck/pattern.h"

struct deck {
    int n_output_devices;
    struct render_target *** rt; // e.g. deck.rt[pattern_num][0]->transform
    struct pattern ** pattern; // e.g. deck.pattern[pattern_num]->intensity
    GLuint * tex_input; // Array of input textures to this deck (one for each render target)
    // e.g. tex_input[render_target_num]
    GLuint * fb_input; // Array of FBOs for accessing input_tex (parallel array)
    // e.g. fb_input[render_target_num]

    GLuint * tex_output; // Which textures are outputs of this deck (textures are not re-allocated, just referenced)
};

void deck_init(struct deck * deck);
void deck_term(struct deck * deck);
void deck_load_pattern(struct deck * deck, int slot, const char * prefix);
void deck_unload_pattern(struct deck * deck, int slot);
void deck_render(struct deck * deck, double time);

#endif
