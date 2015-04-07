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

    int slot_start_x;
    int slot_start_y;
    int slot_width;
    int slot_height;
    int slot_pitch;

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
        int width;
        int height;
        int track_x;
        int track_y;
        int track_width;
        int track_height;
        int handle_start_x;
        int handle_y;
        int handle_width;
        int handle_height;
    } output_slider;

    int pattern_start_x;
    int pattern_start_y;
    int pattern_pitch;
    int pattern_width;
    int pattern_height;
    int pattern_text_x;
    int pattern_text_y;

    struct {
        int start_x;
        int start_y;
        int pitch;
        int width;
        int height;
        int text_x;
        int text_y;
        int output_start_x;
        int output_start_y;
        int slider_start_x;
        int slider_start_y;
        int slider_pitch;
    } signal;
} layout_t;

extern layout_t layout;

void ui_init();
void ui_quit();
void ui_update_master();
void ui_render();
int ui_poll();

#endif
