#ifndef __MIDI_LAYOUT_H_
#define __MIDI_LAYOUT_H_

#include "midi/midi.h"
#include "midi/controllers.h"
#include "patterns/pattern.h"

void midi_setup_layout();

int midi_handle_note_event(struct midi_event * event);

struct midi_note_slot_map {
    int slot_index;
    int device;
    int data1;
    enum pat_event note_on_ev;
    enum pat_event note_off_ev;
    enum pat_event aftertouch_ev;
};

#endif
