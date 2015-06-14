#ifndef __PATTERN_H
#define __PATTERN_H

#include "core/slot.h"
#include "core/time.h"
#include "util/color.h"

struct pattern;
struct slot;

enum pat_event {
    PATEV_NONE = 0,

    PATEV_M1_NOTE_ON,
    PATEV_M1_NOTE_OFF,
    PATEV_M1_AFTERTOUCH, // Not currently used by any of  our midi controllers

    PATEV_M2_NOTE_ON,
    PATEV_M2_NOTE_OFF,
    PATEV_M2_AFTERTOUCH, // Not currently used by any of  our midi controllers

    PATEV_MOUSE_DOWN_X,
    PATEV_MOUSE_DOWN_Y,
    PATEV_MOUSE_DRAG_X, 
    PATEV_MOUSE_DRAG_Y,
    PATEV_MOUSE_UP_X, 
    PATEV_MOUSE_UP_Y,

    PATEV_AUDIO_BEAT, // Not currently used
    PATEV_AUDIO_BAR, // Not currently used
};

typedef void* pat_state_pt;
typedef void (*pat_init_fn_pt)(pat_state_pt);
typedef void (*pat_update_fn_pt)(struct slot* slot, mbeat_t t);
typedef int (*pat_event_fn_pt)(struct slot* slot, enum pat_event event, float event_data);
typedef color_t (*pat_render_fn_pt)(struct slot* slot, float x, float y);

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_event_fn_pt event;
    pat_render_fn_pt render;
    int n_params;
    parameter_t* parameters;
    int state_size;
    char* name;
} pattern_t;

#endif
