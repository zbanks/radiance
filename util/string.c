#include "util/string.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Fall back on library version if it exists
#pragma weak strdup
char * strdup(const char* s) {
    // Returns a copy of the string 
    char* p = (char *) malloc(strlen(s)+1);
    if (p) strcpy(p, s);
    return p;
}

char * rsprintf(const char * fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    int len = vsnprintf(NULL, 0, fmt, vargs) + 1;
    char * buf = malloc(len);
    if(!buf) return 0;

    va_end(vargs);
    va_start(vargs, fmt);

    vsnprintf(buf, len, fmt, vargs);

    va_end(vargs);

    return buf;
}
