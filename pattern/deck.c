#include "pattern/deck.h"
#include "util/err.h"
#include "util/config.h"
#include "util/string.h"
#include "util/ini.h"
#include "util/math.h"
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

int deck_load_pattern(struct deck * deck, int slot, const char * prefix, float intensity) {
    assert(slot >= 0 && slot < config.deck.n_patterns);
    struct pattern * p = calloc(1, sizeof *p);

    if(intensity < 0) {
        if (deck->pattern[slot])
            intensity = deck->pattern[slot]->intensity;
        else
            intensity = 0.;
    }

    if(prefix[0] == '\0') {
        if(deck->pattern[slot]) {
            prefix = deck->pattern[slot]->name;
        } else return -1;
    }

    int result = pattern_init(p, prefix);
    if(result != 0) {
        free(p);
        return result;
    }
    if(deck->pattern[slot]) {
        pattern_term(deck->pattern[slot]);
        free(deck->pattern[slot]);
    }
    deck->pattern[slot] = p;
    p->intensity = intensity;
    return 0;
}

void deck_unload_pattern(struct deck * deck, int slot) {
    assert(slot >= 0 && slot < config.deck.n_patterns);
    if(deck->pattern[slot]) {
        pattern_term(deck->pattern[slot]);
        free(deck->pattern[slot]);
        deck->pattern[slot] = NULL;
    }
}

struct deck_ini_data {
    struct deck * deck;
    const char * name;
    bool found;
};

static int deck_ini_handler(void * user, const char * section, const char * name, const char * value) {
    struct deck_ini_data * data = user;

    if (data->found) return 1;
    if (strcmp(section, "decks") != 0) return 1;
    if (strcmp(name, data->name) != 0) return 1;
    data->found = true;

    char * val = strdup(value);
    if (val == NULL) MEMFAIL();
    
    int slot = 0;
    while (slot < config.deck.n_patterns) {
        char * entry = strsep(&val, " ");
        if (entry == NULL || entry[0] == '\0') break;
        if (entry[0] == '_' && entry[1] == '\0') {
            slot++;
            continue;
        }

        char * name = strsep(&entry, ":");
        float intensity = CLAMP(atof(entry), 0.0, 1.0);

        int rc =  deck_load_pattern(data->deck, slot++, name, intensity);
        if (rc < 0) {
            WARN("Error loading pattern '%s'", name);
            break;
        }
    }
    while (slot < config.deck.n_patterns)
        deck_unload_pattern(data->deck, slot++);

    free(val);
    return 1;
}

int deck_load_set(struct deck * deck, const char * name) {
    struct deck_ini_data data = {
        .deck = deck, .name = name, .found = false
    };
    if (data.name[0] == ':') data.name++;

    int rc = ini_parse(params.paths.decks_config, deck_ini_handler, &data);
    if (!data.found) {
        DEBUG("No deck set named '%s'", name);
        return -1;
    }
    return rc;
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

int deck_save(const struct deck * deck, const char * name) {
    FILE * f = fopen(params.paths.decks_config, "a");
    if (f == NULL) {
        ERROR("Unable to open '%s' for appending", params.paths.decks_config);
        return -1;
    }

    int rc = fprintf(f, "%s=", name);
    if (rc < 0) goto fail;

    for (int i = 0; i < config.deck.n_patterns; i++) {
        if (deck->pattern[i] == NULL)
            rc = fprintf(f, " _");
        else
            rc = fprintf(f, " %s:%0.2f", deck->pattern[i]->name, deck->pattern[i]->intensity);
        if (rc < 0) goto fail;
    }

    rc = fprintf(f, "\n");
    if (rc < 0) goto fail;

    INFO("Saved deck '%s' to '%s'", name, params.paths.decks_config);

fail:
    if (rc < 0)
        ERROR("Unable to write deck '%s' to '%s'", name, params.paths.decks_config);

    fclose(f);
    return rc;
}
