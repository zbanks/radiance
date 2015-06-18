#include <string.h>
#include <stdlib.h>
#include "util/string.h"

// Fall back on library version if it exists
#pragma weak strdup
char * strdup(const char* s) {
    // Returns a copy of the string 
    char* p = (char *) malloc(strlen(s)+1);
    if (p) strcpy(p, s);
    return p;
}

char * strcatdup(const char * first, const char * second){
    // Returns a pointer to a new string, first+second
    size_t len = strlen(first) + strlen(second) + 1;
    char * p = (char *) malloc(len);
    if(p){
        strcpy(p, first);
        strcat(p, second);
    }
    return p;
}
