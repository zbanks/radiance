#ifndef __CORE_CONFIG_MACROS_H__
#define __CORE_CONFIG_MACROS_H__

#define INT int
#define SINT16 Sint16
#define UINT16 Uint16
#define STRING char *
#define FLOAT float

#define INT_FN(x) atoi(x)
#define SINT16_FN(x) atoi(x)
#define UINT16_FN(x) atoi(x)
#define STRING_FN(x) mystrdup(x)
#define FLOAT_FN(x) atof(x)

#define INT_FMT(x) "%d", x
#define SINT16_FMT(x) "%d", x
#define UINT16_FMT(x) "%d", x
#define STRING_FMT(x) "%s", x
#define FLOAT_FMT(x) "%f", x

#define PACKED __attribute__ ((__packed__))

#define CONCAT(x, y) CONCAT2(x, y)
#define CONCAT2(x, y) x ## y

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define EXPAND(...) __VA_ARGS__

/* ISEMPTY macro from https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/ */

#define _ARG16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define HAS_COMMA(...) _ARG16(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define _TRIGGER_PARENTHESIS_(...) ,
 
#define ISEMPTY(...)                                                    \
_ISEMPTY(                                                               \
          /* test if there is just one argument, eventually an empty    \
             one */                                                     \
          HAS_COMMA(__VA_ARGS__),                                       \
          /* test if _TRIGGER_PARENTHESIS_ together with the argument   \
             adds a comma */                                            \
          HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__),                 \
          /* test if the argument together with a parenthesis           \
             adds a comma */                                            \
          HAS_COMMA(__VA_ARGS__ (/*empty*/)),                           \
          /* test if placing it between _TRIGGER_PARENTHESIS_ and the   \
             parenthesis adds a comma */                                \
          HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/))      \
          )
 
#define PASTE5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define _ISEMPTY(_0, _1, _2, _3) HAS_COMMA(PASTE5(_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define _IS_EMPTY_CASE_0001 ,

/* End of ISEMPTY macro code */

#define PREFIX(prefix, suffix) CONCAT(CONCAT(PREFIX_, ISEMPTY(prefix))(prefix), suffix)
#define PREFIX_1(prefix) prefix
#define PREFIX_0(prefix) CONCAT(prefix, _)

#endif // End of include guard

