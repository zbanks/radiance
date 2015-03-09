#ifndef __ERR_H
#define __ERR_H

#define ERROR(x,...) fprintf(stderr,"ERROR: " __FILE__ " line %d: " x, __LINE__, ##__VA_ARGS__)

#endif
