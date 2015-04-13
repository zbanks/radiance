#ifndef __MIDI_H
#define __MIDI_H

#include "core/parameter.h"

#define N_DATA1 256

struct midi_connection_table {
    unsigned char device;
    unsigned char event;
    param_output_t outputs[N_DATA1];
    struct midi_connection_table * next;
};

void midi_connect_param(param_state_t * param, unsigned char device, unsigned char event, unsigned char data1);
void midi_attach_param(param_state_t * param);
void midi_clear_attach();
void midi_start();
void midi_stop();

#endif
