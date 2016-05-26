#include "pattern/deck.h"
#include "util/err.h"
#include "util/config.h"
#include <stdlib.h>
#include <string.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <assert.h>

void deck_init(struct deck * deck) {
    GLenum e;

    memset(deck, 0, sizeof *deck);
    deck->pattern = calloc(config.deck.n_patterns, sizeof *deck->pattern);
    if(deck->pattern == NULL) MEMFAIL();

    glGenTextures(1, &deck->tex_input);
    glGenFramebuffersEXT(1, &deck->fb_input);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindTexture(GL_TEXTURE_2D, deck->tex_input);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.pattern.master_width, config.pattern.master_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, deck->fb_input);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                              deck->tex_input, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void deck_term(struct deck * deck) {
    glDeleteTextures(1, &deck->tex_input);
    glDeleteFramebuffersEXT(1, &deck->fb_input);
    glDeleteTextures(1, &deck->tex_output);

    for(int i = 0; i < config.deck.n_patterns; i++) {
        if(deck->pattern[i] != NULL) {
            pattern_term(deck->pattern[i]);
            free(deck->pattern[i]);
            deck->pattern[i] = NULL;
        }
    }
    memset(deck, 0, sizeof *deck);
}

void deck_load_pattern(struct deck * deck, int slot, const char * prefix) {
    assert(slot >= 0 && slot < config.deck.n_patterns);
    if(deck->pattern[slot]) {
        pattern_term(deck->pattern[slot]);
    } else {
        deck->pattern[slot] = calloc(1, sizeof *deck->pattern[slot]);
    }
    pattern_init(deck->pattern[slot], prefix);
}

void deck_unload_pattern(struct deck * deck, int slot) {
    assert(slot >= 0 && slot < config.deck.n_patterns);
    if(deck->pattern[slot]) {
        pattern_term(deck->pattern[slot]);
        free(deck->pattern[slot]);
        deck->pattern[slot] = NULL;
    }
}

void deck_render(struct deck * deck) {
    deck->tex_output = deck->tex_input;

    for(int i = 0; i < config.deck.n_patterns; i++) {
        if(deck->pattern[i] != NULL) {
            pattern_render(deck->pattern[i], deck->tex_output);
            deck->tex_output = deck->pattern[i]->tex_output;
        }
    }
}
