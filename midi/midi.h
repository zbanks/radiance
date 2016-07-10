#ifndef __MIDI_H
#define __MIDI_H

#include <portmidi.h>
#include <SDL2/SDL.h>

#define SDL_MIDI_COMMAND_EVENT (SDL_USEREVENT)

enum midi_status {
    MIDI_STATUS_NOTEOFF = 0x80,
    MIDI_STATUS_NOTEON = 0x90,
    MIDI_STATUS_CC = 0xB0,
    MIDI_STATUS_AFTERTOUCH = 0xD0,
};

struct midi_controller {
    const char * name;
    const char * short_name;
    PmDeviceID device_id;
    PortMidiStream * stream;

    int enabled;
    struct _controller_midi_config * config;
};

extern struct midi_controller * midi_controllers;
extern int n_midi_controllers;

extern Uint32 midi_command_event;

void midi_start();
void midi_stop();
int midi_refresh_devices();

PmError pm_errmsg(PmError err);

#endif
