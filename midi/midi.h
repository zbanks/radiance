#ifndef __MIDI_H
#define __MIDI_H

#include <portmidi.h>
#include "core/parameter.h"
#include <SDL/SDL.h>

#define N_DATA1 256

#define SDL_MIDI_NOTE_ON (SDL_USEREVENT)
#define SDL_MIDI_NOTE_OFF (SDL_USEREVENT + 1)

#define MIDI_EV_STATUS_MASK 0xF0
enum midi_event_status {
    MIDI_EV_NOTE_OFF = 0x80,
    MIDI_EV_NOTE_ON = 0x90,
    MIDI_EV_KEY_AT = 0xA0, // Polyphonic key pressure (aftertouch)
    MIDI_EV_CTRL_CHANGE = 0xB0,
    MIDI_EV_PROG_CHANGE = 0xC0,
    MIDI_EV_CHAN_AT = 0xD0, // Channel pressure (aftertouch)
    MIDI_EV_PB_CHANGE = 0xE0, // Pitchbend change
    MIDI_EV_RT = 0xF0,
};

struct midi_event {
    unsigned char device;
    unsigned char event;
    unsigned char data1;
    unsigned char data2;
};

struct midi_connection_table {
    unsigned char device;
    unsigned char event;
    param_output_t outputs[N_DATA1];
    //unsigned char events[N_DATA1]; // Semaphores
    struct midi_connection_table * next;
};

#define MIDI_MAX_EVENTS 255

void midi_connect_param(param_state_t * param, unsigned char device, unsigned char event, unsigned char data1);
void midi_attach_param(param_state_t * param);
void midi_clear_attach();
void midi_start();
void midi_stop();

PmError pm_errmsg(PmError err);

#endif
