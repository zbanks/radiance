#ifndef __PATTERN_H
#define __PATTERN_H

#include "core/slot.h"
#include "core/time.h"
#include "util/color.h"

struct pattern;
struct slot;

enum pat_source {
    PATSRC_NONE = 0,
    PATSRC_MOUSE_X,
    PATSRC_MOUSE_Y,
    // PATSRC_AUDIO, // Not yet used, not sure if it will be...
    PATSRC_MIDI_0,
    PATSRC_MIDI_1,
    PATSRC_MIDI_2,
    PATSRC_MIDI_3, // Numbers up to PATSRC_MAX are also valid

    PATSRC_MAX = 1024
};

enum pat_sub_event {
    PATEV_START,  // Mouse down; MIDI note on
    PATEV_MIDDLE, // Mouse drag; MIDI aftertouch
    PATEV_END,    // Mosue up; MIDI note off
};

struct pat_event {
    enum pat_source source;
    enum pat_sub_event event;
};

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();
typedef void (*pat_update_fn_pt)(struct slot* slot, mbeat_t t);
typedef int (*pat_event_fn_pt)(struct slot* slot, struct pat_event event, float event_data);
typedef color_t (*pat_render_fn_pt)(struct slot* slot, float x, float y);
typedef void (*pat_del_fn_pt)(pat_state_pt state);

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_event_fn_pt event;
    pat_render_fn_pt render;
    pat_del_fn_pt del;
    int n_params;
    parameter_t* parameters;
    char* name;
} pattern_t;

#endif
