#ifndef __MIDI_H
#define __MIDI_H

#include <portmidi.h>
#include "core/parameter.h"
#include "core/slot.h"
#include <SDL/SDL.h>
#include <portmidi.h>

#define SDL_MIDI_SLOT_EVENT (SDL_USEREVENT)

enum midi_status {
    MIDI_STATUS_NOTEOFF = 0x80,
    MIDI_STATUS_NOTEON = 0x90,
    MIDI_STATUS_CC = 0xB0,
    MIDI_STATUS_AFTERTOUCH = 0xD0,
};

struct midi_slot_event {
    slot_t * slot;
    struct pat_event event;
};

struct midi_connection {
    unsigned char event;
    unsigned char data1;
    param_state_t * param_state;
    struct midi_slot_event slot_event;
};

#define N_MAX_MIDI_CONNECTIONS 1024

struct midi_controller {
    const char * name;
    const char * short_name;
    PmDeviceID device_id;
    PortMidiStream * stream;

    int enabled;
    int n_connections;
    struct midi_connection connections[N_MAX_MIDI_CONNECTIONS];
};

extern struct midi_controller * midi_controllers;
extern int n_midi_controllers;

void midi_connect_param(param_state_t * param, unsigned char device, unsigned char event, unsigned char data1);
//void midi_attach_param(param_state_t * param);
void midi_clear_attach();
void midi_start();
void midi_stop();
int midi_refresh_devices();

PmError pm_errmsg(PmError err);

#endif
