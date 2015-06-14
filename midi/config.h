#ifndef __MIDI_CONFIG_H__
#define __MIDI_CONFIG_H__

#include "midi/midi.h"

int midi_config_load(const char * filename, struct midi_controller * controllers, int n_controllers);
#endif
