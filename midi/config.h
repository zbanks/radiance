#pragma once
#include "util/common.h"

NOT_CXX
#include "util/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif
#define CFGOBJ midi_config
#define CFGOBJ_PATH "midi/midi_map.def"

#include "util/config_gen_h.def"

extern struct midi_config midi_config;

CXX_OK
