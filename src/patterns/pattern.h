#ifndef __PATTERN_H
#define __PATTERN_H

#include "core/time.h"
#include "util/color.h"
#include "core/parameter.h"

struct slot;

typedef enum pat_command_status
{
    STATUS_START,  // e.g. Mouse down; MIDI note on
    STATUS_CHANGE, // e.g. Mouse drag; MIDI aftertouch
    STATUS_STOP,   // e.g. Mouse up; MIDI note off
} pat_command_status_t;

typedef struct pat_command
{
    int index;
    pat_command_status_t status;
    float value;
} pat_command_t;

typedef void* pat_state_pt;
typedef void (*pat_init_fn_pt)(pat_state_pt);
typedef void (*pat_update_fn_pt)(struct slot* slot, mbeat_t t);
typedef void (*pat_command_fn_pt)(struct slot* slot, pat_command_t cmd);
typedef color_t (*pat_render_fn_pt)(const pat_state_pt restrict state, float x, float y);

typedef struct pattern
{
    pat_init_fn_pt init;
    pat_update_fn_pt update;
    pat_command_fn_pt command;
    pat_render_fn_pt render;
    int n_params;
    const parameter_t* parameters;
    int state_size;
    const char* name;
} pattern_t;

#define MAKE_PATTERN { \
    .render = (pat_render_fn_pt)&render, \
    .init = (pat_init_fn_pt)&init, \
    .update = &update, \
    .command = &command, \
    .n_params = N_PARAMS, \
    .parameters = params, \
    .state_size = sizeof(state_t), \
    .name = name, \
}

#endif
