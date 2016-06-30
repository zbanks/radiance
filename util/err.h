#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _ERR_STRINGIFY2(x) #x
#define _ERR_STRINGIFY(x) _ERR_STRINGIFY2(x)

#define DEBUG_INFO __FILE__ ":" _ERR_STRINGIFY(__LINE__) ":" _ERR_STRINGIFY(__func__)

#define _ERR_MSG(severity, msg, ...) fprintf(stderr, "[%-5s] [%s:%s:%d] " msg "\n", severity, __FILE__, __func__, __LINE__, ## __VA_ARGS__)
#define FAIL(...) ({ERROR(__VA_ARGS__); exit(EXIT_FAILURE);})
#define ERROR(...) _ERR_MSG("error", ## __VA_ARGS__)
#define WARN(...)  _ERR_MSG("warn",  ## __VA_ARGS__)
#define INFO(...)  _ERR_MSG("info",  ## __VA_ARGS__)
#define DEBUG(...) _ERR_MSG("debug", ## __VA_ARGS__)
#define MEMFAIL() FAIL("Could not allocate memory")

#define FAIL_P(...) ({ERROR(__VA_ARGS__); exit(EXIT_FAILURE);})
#define ERROR_P(msg, ...) _ERR_MSG("error","[%s] ", strerror(errno), ## __VA_ARGS__)

/*
#include <execinfo.h>
#define BACKTRACE() ({ \
    INFO("Backtrace:"); \
    void * _buffer[100]; \
    int _nptrs = backtrace(_buffer, 100); \
    backtrace_symbols_fd(_buffer, _nptrs, fileno(stderr)); \
})
*/
