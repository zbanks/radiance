#ifndef __UI_H
#define __UI_H

#include "core/slot.h"

typedef struct {
    int win_width;
    int win_height;

    int master_x;
    int master_y;
    int master_width;
    int master_height;

    struct {
        int start_x;
        int start_y;
        int width;
        int height;
        int pitch;
    } slot;

    int preview_x;
    int preview_y;
    int preview_width;
    int preview_height;

    int alpha_pitch;
    int alpha_slider_x;
    int alpha_slider_y;
    int alpha_slider_height;
    int alpha_handle_x;
    int alpha_handle_y;
    int alpha_handle_width;
    int alpha_handle_height;

    struct {
        int slider_start_x;
        int slider_start_y;
        int slider_pitch;
    } pattern;
 
    struct {
        int width;
        int height;
        int name_x;
        int name_y;
        int source_end_x;
        int source_y;
        int track_x;
        int track_y;
        int track_width;
        int handle_start_x;
        int handle_y;
        int handle_width;
        int handle_height;
        int value_start_x;
        int value_start_y;
    } slider;

    struct {
        int start_x;
        int start_y;
        int pitch_x;
        int pitch_y;
        int width;
        int height;
        int text_x;
        int text_y;
    } add_pattern;

    struct {
        int start_x;
        int start_y;
        int pitch_x;
        int pitch_y;
        int width;
        int height;
        int text_x;
        int text_y;
    } add_hit;

    struct {
        int start_x;
        int start_y;
        int pitch;
        int width;
        int height;
    } hit_slot;

    struct {
        int start_x;
        int start_y;
        int pitch;
        int width;
        int height;
        int text_x;
        int text_y;
        int output_x;
        int output_y;
        int slider_start_x;
        int slider_start_y;
        int slider_pitch;
    } signal;

    struct {
        int start_x;
        int start_y;
        int width;
        int height;
        int pitch_x;
        int pitch_y;
        int text_x;
        int text_y;
        int graph_x;
        int graph_y;
    } filter;

    struct {
        int width;
        int height;
        int scroll_rate;
    } graph_signal;

    struct {
        int width;
        int height;
        int scroll_rate;
    } graph_filter;
} layout_t;

extern layout_t layout;
extern param_state_t * active_param_source;

void ui_init();
void ui_quit();
void ui_update_master();
void ui_render();
int ui_poll();

#endif
