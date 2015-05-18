#ifndef __ERR_H
#define __ERR_H

#include <stdlib.h>
#include <stdio.h>

#define ERROR(x,...) fprintf(stderr,"ERROR: " __FILE__ " line %d: " x, __LINE__, ##__VA_ARGS__)
#define FAIL(...) {ERROR(__VA_ARGS__); exit(EXIT_FAILURE);}

#define UNUSED __attribute__ ((unused))

#endif
