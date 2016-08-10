#pragma once
#include "util/common.h"
#include "pattern/pattern.h"
struct deck {
    std::vector<std::unique_ptr<pattern> > patterns;
    GLuint tex_input{0};
    GLuint fb_input{0};
    GLuint tex_output{0};
    deck();
    virtual ~deck();
    void init();
    void term();
    int load_pattern(int slot, const char *prefix);
    int load_set(const char *prefix);
    void render();
    void unload_pattern(int slot);
};
