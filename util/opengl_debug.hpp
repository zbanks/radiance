#pragma once

#include "util/common.h"


constexpr const char *uiDebugType(GLenum type)
{
    switch(type) {
#define CASE(x) case GL_DEBUG_TYPE_ ## x: return #x
        CASE(ERROR);
        CASE(DEPRECATED_BEHAVIOR);
        CASE(UNDEFINED_BEHAVIOR);
        CASE(PORTABILITY);
        CASE(PERFORMANCE);
        CASE(MARKER);
        CASE(PUSH_GROUP);
        CASE(POP_GROUP);
        default: return "UNKNOWN TYPE";
#undef CASE
    }
}
constexpr const char *uiDebugSource(GLenum type)
{
#define CASE(x) case GL_DEBUG_SOURCE_ ## x: return #x
    switch(type) {
        CASE(API);
        CASE(WINDOW_SYSTEM);
        CASE(SHADER_COMPILER);
        CASE(THIRD_PARTY);
        CASE(APPLICATION);
        CASE(OTHER);
        default: return "UNKNOWN SOURCE";
    }
#undef CASE
}
constexpr const char * uiDebugSeverity(GLenum sev)
{

#define CASE(x) case GL_DEBUG_SEVERITY_ ## x: return #x
    switch(sev){
        CASE(HIGH);
        CASE(MEDIUM);
        CASE(LOW);
        CASE(NOTIFICATION);
        default: return "UNKNOWN SEVERITY";
    }
#undef CASE
}
constexpr loglevel to_loglevel(GLenum severity)
{
#define CASE(x,y) case GL_DEBUG_SEVERITY_ ## x: return LOGLEVEL_ ## y
    switch(severity){
        CASE(HIGH,ERROR);
        CASE(MEDIUM,INFO);
        CASE(LOW,DEBUG);
        CASE(NOTIFICATION,ALL);
        default: return LOGLEVEL_ALL;
    }
#undef CASE
}
