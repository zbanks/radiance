#ifndef __MIDI_CONFIG_H__
#define __MIDI_CONFIG_H__

#include "signals/signal.h"
#include "core/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ midi_map
#define CFGOBJ_PATH "midi/midi_map.def"

#include "core/config_gen_h.def"

int midi_config_load(const char * filename, struct midi_controller * controllers, int n_controllers);
#endif
