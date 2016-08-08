#pragma once
#include "util/common.h"
#include "output/slice.h"
#include "util/config_macros.h"
NOT_CXX
#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif
#define CFGOBJ output_config
#define CFGOBJ_PATH "output/config.def"

// Color type - depends on SDL_Color
#define COLOR SDL_Color
#define COLOR_PARSE(x) _parse_color(x)
#define COLOR_FORMAT(x) "#%02hhx%02hhx%02hhx", x.r, x.g, x.b
#define COLOR_FREE(x) (void)(x)
#define COLOR_PREP(x) _parse_color(x)

static inline SDL_Color _parse_color(const char * cstr){
    SDL_Color out;
    if(sscanf(cstr, "#%02hhx%02hhx%02hhx", &out.r, &out.g, &out.b) == 3)
        return out;
    return (SDL_Color) {0, 0, 0, 0};
}

// Lux address type: int in '0xdeadbeef' form
#define LUXADDR uint32_t
#define LUXADDR_PARSE(x) strtol(x, NULL, 16)
#define LUXADDR_FORMAT(x) "%#08x", x
#define LUXADDR_FREE(x) (void)(x)
#define LUXADDR_PREP(x) x

// Vertex list, e.g. "-1 1,1 1,1 -1,-1 -1"
#define VERTEXLIST struct output_vertex *
#define VERTEXLIST_PARSE(x) output_vertex_list_parse(x)
#define VERTEXLIST_FORMAT(x) "%s", output_vertex_list_serialize(x)
#define VERTEXLIST_FREE(x) output_vertex_list_destroy(x)
#define VERTEXLIST_PREP(x) output_vertex_list_parse(x)

#include "util/config_gen_h.def"

extern struct output_config output_config;

CXX_OK
