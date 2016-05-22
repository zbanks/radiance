#include "deck/deck.h"
#include "util/err.h"
#include "util/config.h"
#include <stdlib.h>
#include <string.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <assert.h>

#define NON_OUTPUT_RT 1

static const double identity[9] = {1, 0, 0,
                                   0, 1, 0,
                                   0, 0, 1};

void deck_init(struct deck * deck) {
    memset(deck, 0, sizeof *deck);
    deck->n_output_devices = 0; // TODO change this
    deck->rt = calloc(config.deck.n_patterns, sizeof *deck->rt);
    if(deck->rt == NULL) MEMFAIL();
    for(int i = 0; i < config.deck.n_patterns; i++) {
        deck->rt[i] = calloc(NON_OUTPUT_RT + deck->n_output_devices, sizeof *deck->rt[i]);
        if(deck->rt[i] == NULL) MEMFAIL();

        deck->rt[i][0] = calloc(1, sizeof *deck->rt[i][0]);
        if(deck->rt[i][0] == NULL) MEMFAIL();
        pattern_render_target_init(deck->rt[i][0], config.deck.preview_width, config.deck.preview_height, identity);
        // TODO output device render target initialization
    }
    deck->pattern = calloc(config.deck.n_patterns, sizeof *deck->pattern);
    if(deck->pattern == NULL) MEMFAIL();

    deck->tex_input = calloc(NON_OUTPUT_RT + deck->n_output_devices, sizeof *deck->tex_input);
    if(deck->tex_input == NULL) MEMFAIL();
    deck->fb_input = calloc(NON_OUTPUT_RT + deck->n_output_devices, sizeof *deck->fb_input);
    if(deck->fb_input == NULL) MEMFAIL();

    glGenTextures(1, deck->tex_input);
    glGenFramebuffersEXT(1, deck->fb_input);

    glBindTexture(GL_TEXTURE_2D, deck->tex_input[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.deck.preview_width, config.deck.preview_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // TODO output device input texture initialization

    for(int i = 0; i < deck->n_output_devices + 1; i++) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, deck->fb_input[i]);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                                  deck->tex_input[i], 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    deck->tex_output = calloc(NON_OUTPUT_RT + deck->n_output_devices, sizeof *deck->tex_output);
    if(deck->tex_output == NULL) MEMFAIL();

    GLenum e;
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void deck_term(struct deck * deck) {
    glDeleteFramebuffersEXT(NON_OUTPUT_RT + deck->n_output_devices, deck->tex_input);
    free(deck->tex_input);
    deck->tex_input = NULL;
    glDeleteTextures(NON_OUTPUT_RT + deck->n_output_devices, deck->tex_input);
    free(deck->fb_input);
    deck->fb_input = NULL;
    free(deck->tex_output);
    deck->tex_output = NULL;

    for(int i = 0; i < config.deck.n_patterns; i++) {
        if(deck->pattern[i] != NULL) {
            pattern_term(deck->pattern[i]);
            free(deck->pattern[i]);
        }

        for(int j = 0; j < NON_OUTPUT_RT + deck->n_output_devices; j++) {
            pattern_render_target_term(deck->rt[i][j]);
            free(deck->rt[i][j]);
            deck->rt[i][j] = 0;
        }
        free(deck->rt[i]);
    }
    memset(deck->rt, 0, config.deck.n_patterns * sizeof *deck->rt);
}

void deck_load_pattern(struct deck * deck, int slot, const char * prefix) {
    assert(slot >= 0 && slot < config.deck.n_patterns);
    if(deck->pattern[slot]) {
        pattern_term(deck->pattern[slot]);
    } else {
        deck->pattern[slot] = calloc(1, sizeof *deck->pattern[slot]);
    }
    pattern_init(deck->pattern[slot], prefix, deck->rt[slot], NON_OUTPUT_RT + deck->n_output_devices);
}

void deck_unload_pattern(struct deck * deck, int slot) {
    assert(slot >= 0 && slot < config.deck.n_patterns);
    if(deck->pattern[slot]) {
        pattern_term(deck->pattern[slot]);
        free(deck->pattern[slot]);
        deck->pattern[slot] = NULL;
    }
}

void deck_render(struct deck * deck, double time) {
    for(int i = 0; i < NON_OUTPUT_RT + deck->n_output_devices; i++) {
        deck->tex_output[i] = deck->tex_input[i];
    }

    for(int i = 0; i < config.deck.n_patterns; i++) {
        if(deck->pattern[i] != NULL) {
            pattern_render(deck->pattern[i], time, deck->tex_output);
            for(int j = 0; j < NON_OUTPUT_RT + deck->n_output_devices; j++) {
                deck->tex_output[j] = deck->rt[i][j]->tex_screen[0];
            }
        }
    }
}
