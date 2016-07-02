#pragma once

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))
#define INTERP(a, low, high) ((a) * (low) + (1.0 - (a)) * (high))
