#pragma once
#include "util/common.h"
#include "pattern/deck.h"
NOT_CXX
struct crossfader {
    bool left_on_top;

    GLuint shader;
    GLuint tex_output;
    GLuint fb;

    float position;
    uint8_t * rb_buf;
};

void crossfader_init(struct crossfader * crossfader);
void crossfader_term(struct crossfader * crossfader);
void crossfader_render(struct crossfader * crossfader, GLuint left, GLuint right);

CXX_OK
