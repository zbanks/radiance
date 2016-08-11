#include "util/common.h"

#include "pattern/deck.h"
#include "util/err.h"
#include "util/config.h"
#include "util/string.h"
#include "util/ini.h"

deck::deck() = default;

void deck::init()
{
    GLenum e;
    patterns.resize(config.deck.n_patterns);

    glGenTextures(1, &tex_input);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
    glGenFramebuffers(1, &fb_input);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindTexture(GL_TEXTURE_2D, tex_input);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.pattern.master_width, config.pattern.master_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    glBindFramebuffer(GL_FRAMEBUFFER, fb_input);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                              tex_input, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void deck::term()
{
    if(tex_input)
        glDeleteTextures(1, &tex_input);
    if(fb_input)
        glDeleteFramebuffers(1, &fb_input);
    if(tex_output)
        glDeleteTextures(1, &tex_output);
    tex_input = 0;
    fb_input = 0;
    tex_output = 0;
    patterns.clear();
}
deck::~deck()
{
    term();
}
int deck::load_pattern(int slot, const char * prefix)
{
    assert(slot >= 0 && slot < config.deck.n_patterns);
    float intensity = 0;

    if(patterns[slot])
        intensity = patterns[slot]->intensity;

    if(prefix[0] == '\0') {
        if(patterns[slot]) {
            prefix = patterns[slot]->name.c_str();
        } else {
            return -1;
        }
    }
    try {
        auto p = std::make_unique<pattern>(prefix);
        p->intensity = intensity;
        patterns[slot].swap(p);
    }catch(const std::exception &e) {
        ERROR("failed to load pattern: %s",e.what());
    }
    return 0;
}

void deck::unload_pattern(int slot)
{
    assert(slot >= 0 && slot < config.deck.n_patterns);
    patterns[slot].reset();
}

struct deck_ini_data {
    struct deck * deck;
    const char * name;
    bool found;
};

static int deck_ini_handler(void * user, const char * section, const char * name, const char * value) {
    struct deck_ini_data * data = static_cast<deck_ini_data*>(user);

    INFO("%s %s %s", section, name, value);
    if (data->found) return 1;
    if (strcmp(section, "decks") != 0) return 1;
    if (strcmp(name, data->name) != 0) return 1;
    data->found = true;

    char * val = strdup(value);
    if (val == NULL) MEMFAIL();
    
    int slot = 0;
    while (slot < config.deck.n_patterns) {
        char * prefix = strsep(&val, " ");
        if (prefix == NULL || prefix[0] == '\0') break;
        auto rc = data->deck->load_pattern(slot++, prefix);
        if (rc < 0) return 0;
    }
    free(val);
    return 1;
}

int deck::load_set(const char * name)
{
    for (int slot = 0; slot < config.deck.n_patterns; slot++)
        unload_pattern(slot);

    struct deck_ini_data data = {
        .deck = this, .name = name, .found = false
    };
    auto rc = ini_parse(config.paths.decks_config, deck_ini_handler, &data);
    if (!data.found) ERROR("No deck set named '%s'", name);
    return rc;
}

void deck::render() {
    tex_output = tex_input;
    for(auto &pat : patterns) {
        if(pat) {
            pat->render(tex_output);
            tex_output = pat->tex_output;
        }
    }
}
