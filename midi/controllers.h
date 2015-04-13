#ifndef __CONTROLLERS_H
#define __CONTROLLERS_H

#include <SDL/SDL.h>

struct midi_controller {
    char * name;
    char * short_name;
    SDL_Color color;
    int enabled;
    int available;
    int n_inputs;
    char ** input_labels;
};

enum midi_devices {
    MIDI_NK2_1,
    MIDI_NP2_1,

    N_MIDI_DEVICES, 
};

extern struct midi_controller controllers_enabled[];
extern int n_controllers_enabled;

extern char * nanokontrol2_labels[];
extern char * nanopad2_labels[];

enum nanokontrol2_inputs {
    NK2_S0 = 0, 
    NK2_S1,
    NK2_S2,
    NK2_S3,
    NK2_S4,
    NK2_S5,
    NK2_S6,
    NK2_S7,

    NK2_K0 = 16,
    NK2_K1,
    NK2_K2,
    NK2_K3,
    NK2_K4,
    NK2_K5,
    NK2_K6,
    NK2_K7,

    NK2_SS0 = 32,
    NK2_SS1,
    NK2_SS2,
    NK2_SS3,
    NK2_SS4,
    NK2_SS5,
    NK2_SS6,
    NK2_SS7,

    NK2_PLAY = 41,
    NK2_STOP,
    NK2_REWIND,
    NK2_FASTFWD,
    NK2_RECORD,
    NK2_CYCLE,

    NK2_MS0 = 48,
    NK2_MS1,
    NK2_MS2,
    NK2_MS3,
    NK2_MS4,
    NK2_MS5,
    NK2_MS6,
    NK2_MS7,

    NK2_LEFT = 58,
    NK2_RIGHT,
    NK2_SET,
    NK2_SLEFT,
    NK2_SRIGHT,

    NK2_RS0 = 64,
    NK2_RS1,
    NK2_RS2,
    NK2_RS3,
    NK2_RS4,
    NK2_RS5,
    NK2_RS6,
    NK2_RS7,

    N_NK2_INPUTS,
};

enum nanopad2_inputs {
    N_NP2_INPUTS,
};
#endif
