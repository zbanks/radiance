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
    PATEV_M1_AFTERTOUCH,

    PATEV_M2_NOTE_ON_2,
    PATEV_M2_NOTE_OFF_2,
    PATEV_M2_AFTERTOUCH_2,

    PATEV_MOUSE_CLICK_X,
    PATEV_MOUSE_CLICK_Y,

    PATEV_AUDIO_BEAT,
    PATEV_AUDIO_BAR,
};

typedef void* pat_state_pt;
typedef pat_state_pt (*pat_init_fn_pt)();
typedef void (*pat_update_fn_pt)(struct slot* slot, mbeat_t t);
typedef int (*pat_event_fn_pt)(struct slot* slot, enum pat_event event, float event_data);
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
