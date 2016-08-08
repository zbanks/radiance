#include "util/string.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

char * rsprintf(const char * fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    int len = vsnprintf(NULL, 0, fmt, vargs) + 1;
    char * buf = malloc(len);
    if(!buf) return NULL;

    va_end(vargs);
    va_start(vargs, fmt);

    vsnprintf(buf, len, fmt, vargs);

    va_end(vargs);

    return buf;
}
