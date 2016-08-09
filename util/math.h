#pragma once

#include "util/common.h"
#ifdef __cplusplus
#include <cmath>
#include <numeric>
#include <algorithm>
#include <utility>
#include <cfloat>
#include <climits>
#include <limits>
#include <functional>

template<class... Ts>
using common_t = std::common_type_t<Ts...>;

template<class T, class U>
constexpr common_t<T,U> MAX(const T& t, const U& u)
{
    return std::max<common_t<T,U> >(t,u);
}
template<class T, class U>
constexpr common_t<T,U> MIN(const T& t, const U& u)
{
    return std::min<common_t<T,U> >(t,u);
}
template<class T, class U>
constexpr U CLAMP(T t, U _min, U _max)
{
    using C = common_t<T, U>;
    return static_cast<U>(std::max<C>(_min,std::min<C>(t, _max)));
}
template<class T>
constexpr T INTERP(auto a, T low, T high) { return static_cast<T>( a * low + (1 - a) * high); }
#else
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))
#define INTERP(a, low, high) ((a) * (low) + (1.0 - (a)) * (high))
#endif
