#include "util/common.h"

#include "pattern/deck.h"
#include "util/glsl.h"
#include "util/err.h"
#include "util/config.h"
#include "util/string.h"
#include "util/ini.h"

deck::deck() = default;

void deck::init()
{
    patterns.resize(config.deck.n_patterns);

    tex_input = make_texture( config.pattern.master_width, config.pattern.master_height);
//    glGenFramebuffers(1, &fb_input);
//    glBindFramebuffer(GL_FRAMEBUFFER,fb_input);
//    glNamedFramebufferTexture(fb_input, GL_COLOR_ATTACHMENT0, tex_input, 0);
    glClearTexImage(tex_input, 0, GL_RGBA,GL_FLOAT,nullptr);
    CHECK_GL();
}

void deck::term()
{
    if(tex_input)
        glDeleteTextures(1, &tex_input);
//    if(fb_input)
//        glDeleteFramebuffers(1, &fb_input);
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
    auto val = std::string{value};
    auto slot = 0;
    while (slot < config.deck.n_patterns) {
        auto seppos = val.find(' ');
        if(val.empty() || seppos == 0)
            break;
        auto prefix = val.substr(0,seppos);
        val = val.substr(seppos+1);
        if(data->deck->load_pattern(slot++, prefix.c_str()) < 0)
            return 0;
    }
    return 1;
}
int deck::load_set(const char * name)
{
    for (auto slot = 0; slot < config.deck.n_patterns; slot++)
        unload_pattern(slot);

    deck_ini_data data = {
        .deck = this, .name = name, .found = false
    };
    auto rc = ini_parse(config.paths.decks_config, deck_ini_handler, &data);
    if (!data.found)
        ERROR("No deck set named '%s'", name);
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
