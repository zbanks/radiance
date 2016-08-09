#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "util/common.h"
NOT_CXX
#include "util/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ config
#define CFGOBJ_PATH "util/config.def"

#include "util/config_gen_h.def"

extern struct config config;

CXX_OK
#endif  // End of include guard
