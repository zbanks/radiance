_Pragma("once")

#ifdef __cplusplus
#define NOT_CXX \
extern "C" {
#define CXX_OK \
}
#else
#define NOT_CXX
#define CXX_OK
#endif
#include <GL/gl3w.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include <GL/glut.h>
#include <GL/glu.h>

#include <semaphore.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

#ifdef __cplusplus
#include <thread>
#include <atomic>

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cfloat>
#include <cinttypes>

#include <utility>
#include <functional>
#include <memory>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>
#include <limits>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <clocale>
#else
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdatomic.h>
#endif

#include "util/err.h"

