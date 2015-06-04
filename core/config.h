#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "core/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ config
#define CFGOBJ_PATH "core/config.def"

#include "core/config_gen_h.def"

#endif  // End of include guard
