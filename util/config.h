#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "util/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ config
#define CFGOBJ_PATH "util/config.def"

// Run-time constants

#include "util/config_gen_h.def"
#undef CFGOBJ
#undef CFGOBJ_PATH

extern struct config config;

// Reloadable parameters

#define CFGOBJ params
#define CFGOBJ_PATH "util/params.def"

extern struct params params;

#include "util/config_gen_h.def"
#undef CFGOBJ
#undef CFGOBJ_PATH

int params_refresh();

#endif  // End of include guard
