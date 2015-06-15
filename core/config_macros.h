#ifndef __CORE_CONFIG_MACROS_H__
#define __CORE_CONFIG_MACROS_H__

#define N_MAX_LIST_ELEMENTS 32

// LIST_NAME(slot) --> slots
// LIST_N_NAME(slot) --> n_slots
// LIST_PREFIX(slot) --> "slot_"
#define LIST_NAME(name) name ## s
#define LIST_N_NAME(name) n_ ## name ## s
#define LIST_PREFIX(name) STRINGIFY(name) "_"

// Wrap default value in PARSE(...) to trigger parse, required for some types

// Numeric types
#define INT int
#define SINT16 Sint16
#define UINT16 Uint16
#define FLOAT float

#define INT_PARSE(x) atoi(x)
#define SINT16_PARSE(x) atoi(x)
#define UINT16_PARSE(x) atoi(x)
#define FLOAT_PARSE(x) atof(x)

#define INT_FORMAT(x) "%d", x
#define SINT16_FORMAT(x) "%d", x
#define UINT16_FORMAT(x) "%d", x
#define FLOAT_FORMAT(x) "%f", x

#define INT_FREE(x) (void)(x)
#define SINT16_FREE(x) (void)(x)
#define UINT16_FREE(x) (void)(x)
#define FLOAT_FREE(x) (void)(x)

#define STRING char *
#define STRING_PARSE(x) mystrdup(x)
#define STRING_FORMAT(x) "%s", x
#define STRING_FREE(x) free(x)

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

