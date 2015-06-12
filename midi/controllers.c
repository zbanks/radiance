#include "controllers.h"

#define N_CONTROLLERS 3

struct midi_controller controllers_enabled[N_MIDI_DEVICES] = {
    [MIDI_NK2_1] = {
        .name = "nanoKONTROL2 MIDI 1",
        .short_name = "NK",
        .enabled = 1,
        .color = {0, 150, 255, 255},
        .n_inputs = N_NK2_INPUTS,
        .input_labels = nanokontrol2_labels,
        .suffix = '\0',
    },
    [MIDI_NP2_1] = {
        .name = "nanoPAD2 MIDI 1",
        .short_name = "NP",
        .enabled = 1,
        .color = {150, 0, 255, 255},
        .n_inputs = N_NP2_INPUTS,
        .input_labels = nanopad2_labels,
        .suffix = '\0',
    },
    [MIDI_NK2_2] = {
        .name = "nanoKONTROL2 MIDI 1",
        .short_name = "nk'",
        .enabled = 1,
        .color = {150, 150, 255, 255},
        .n_inputs = N_NK2_INPUTS,
        .input_labels = nanokontrol2_labels,
        .suffix = '\'',
    },
};

int n_controllers_enabled = N_MIDI_DEVICES;

char * nanokontrol2_labels[N_NK2_INPUTS] = {
    [NK2_S0] = "s1",
    [NK2_S1] = "s2",
    [NK2_S2] = "s3",
    [NK2_S3] = "s4",
    [NK2_S4] = "s5",
    [NK2_S5] = "s6",
    [NK2_S6] = "s7",
    [NK2_S7] = "s8",

    [NK2_K0] = "k1",
    [NK2_K1] = "k2",
    [NK2_K2] = "k3",
    [NK2_K3] = "k4",
    [NK2_K4] = "k5",
    [NK2_K5] = "k6",
    [NK2_K6] = "k7",
    [NK2_K7] = "k8",

    [NK2_SS0] = "S1",
    [NK2_SS1] = "S2",
    [NK2_SS2] = "S3",
    [NK2_SS3] = "S4",
    [NK2_SS4] = "S5",
    [NK2_SS5] = "S6",
    [NK2_SS6] = "S7",
    [NK2_SS7] = "S8",

    [NK2_MS0] = "M1",
    [NK2_MS1] = "M2",
    [NK2_MS2] = "M3",
    [NK2_MS3] = "M4",
    [NK2_MS4] = "M5",
    [NK2_MS5] = "M6",
    [NK2_MS6] = "M7",
    [NK2_MS7] = "M8",

    [NK2_RS0] = "R1",
    [NK2_RS1] = "R2",
    [NK2_RS2] = "R3",
    [NK2_RS3] = "R4",
    [NK2_RS4] = "R5",
    [NK2_RS5] = "R6",
    [NK2_RS6] = "R7",
    [NK2_RS7] = "R8",

    [NK2_PLAY] = "Play",
    [NK2_STOP] = "Stop",
    [NK2_REWIND] = "<<",
    [NK2_FASTFWD] = ">>",
    [NK2_RECORD] = "Rec",
    [NK2_CYCLE] = "Cycle",

    [NK2_LEFT] = "<-T",
    [NK2_RIGHT] = "T->",
    [NK2_SET] = "MSet",
    [NK2_SLEFT] = "<-M",
    [NK2_SRIGHT] = "M->",
};

char * nanopad2_labels[N_NP2_INPUTS] = {
    [NP2_XY_X] = "X",
    [NP2_XY_Y] = "Y",
    [NP2_XY_DOWN] = "XY",

    [NP2_PAD_0A] = "A1",
    [NP2_PAD_0B] = "B1",
    [NP2_PAD_1A] = "A2",
    [NP2_PAD_1B] = "B2",
    [NP2_PAD_2A] = "A3",
    [NP2_PAD_2B] = "B3",
    [NP2_PAD_3A] = "A4",
    [NP2_PAD_3B] = "B4",
    [NP2_PAD_4A] = "A5",
    [NP2_PAD_4B] = "B5",
    [NP2_PAD_5A] = "A6",
    [NP2_PAD_5B] = "B6",
    [NP2_PAD_6A] = "A7",
    [NP2_PAD_6B] = "B7",
    [NP2_PAD_7A] = "A8",
    [NP2_PAD_7B] = "B8",
};
