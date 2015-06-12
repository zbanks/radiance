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
    char suffix;
};

enum midi_devices {
    MIDI_NK2_1,
    MIDI_NP2_1,
    MIDI_NK2_2,

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

// 176: 1 -x , 2 - y, 16 on/off
// 128: down
// 144: up
// 36-51
enum nanopad2_inputs {
    NP2_XY_X = 1,
    NP2_XY_Y,
    NP2_XY_DOWN = 16,

    NP2_PAD_0A = 36,
    NP2_PAD_0B,
    NP2_PAD_1A,
    NP2_PAD_1B,
    NP2_PAD_2A,
    NP2_PAD_2B,
    NP2_PAD_3A,
    NP2_PAD_3B,
    NP2_PAD_4A,
    NP2_PAD_4B,
    NP2_PAD_5A,
    NP2_PAD_5B,
    NP2_PAD_6A,
    NP2_PAD_6B,
    NP2_PAD_7A,
    NP2_PAD_7B,

    N_NP2_INPUTS,
};

#define NP2_PAD_NA(x) (NP2_PAD_0A + (x) * 2)
#define NP2_PAD_NB(x) (NP2_PAD_0B + (x) * 2)

#endif
