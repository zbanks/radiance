#include "pattern/crossfader.h"
#include "util/glsl.h"
#include "util/string.h"
#include "util/err.h"
#include <string.h>

void crossfader_init(struct crossfader * crossfader) {
    memset(crossfader, 0, sizeof *crossfader);
}

void crossfader_term(struct crossfader * crossfader) {
    memset(crossfader, 0, sizeof *crossfader);
}

GLuint deck_crossfader_render(struct crossfader * crossfader, GLuint left, GLuint right) {
    return crossfader->tex_output;
}
