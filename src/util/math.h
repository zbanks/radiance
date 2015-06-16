#ifndef __UTIL_MATH_H__
#define __UTIL_MATH_H__

#define SIN(x) sinf(x)
#define COS(x) cosf(x)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define ABS(x) ((x) > 0 ? (x) : -(x))

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/*
inline static float _sin(float f){
    if(f > M_PI || f < M_PI){
        f = fmod(f + M_PI, M_PI * 2) - M_PI;
    }
    float f2 = f * f;
    float fp = f * f2;
    float x = f - fp / 6.;
    fp *= f2;
    x += fp / 120.;
    fp *= f2;
    x -= fp / (5040.);
    return x;
}

inline static float _cos(float f){
    f += M_PI / 2;
    return _sin(f);
}
*/

#endif
