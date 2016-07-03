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

// Fall back on library version if it exists
#pragma weak strdup
char * strdup(const char* s) {
    // Returns a copy of the string 
    char* p = (char *) malloc(strlen(s)+1);
    if (p) strcpy(p, s);
    return p;
}

#pragma weak strsep
char * strsep(char ** s, const char * delim) {
    if (*s == NULL) return NULL;
    char * original = *s;
    char * next = strpbrk(*s, delim);
    if (next != NULL && *next != '\0')
        *next++ = '\0';
    *s = next;
    return original;
}
