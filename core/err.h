#ifndef __ERR_H
#define __ERR_H

#include <stdlib.h>
#include <stdio.h>

#define _ERR_STRINGIFY2(x) #x
#define _ERR_STRINGIFY(x) _ERR_STRINGIFY2(x)

#define WARN(...) fprintf(stderr,"Warning: " __FILE__ " line " _ERR_STRINGIFY(__LINE__) ": " __VA_ARGS__)
#define ERROR(...) fprintf(stderr,"ERROR: " __FILE__ " line " _ERR_STRINGIFY(__LINE__) ": " __VA_ARGS__)
#define FAIL(...) {ERROR(__VA_ARGS__); exit(EXIT_FAILURE);}

#define UNUSED(x) ((void)(x))

#endif
