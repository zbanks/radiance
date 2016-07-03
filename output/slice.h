#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

struct output_pixels {
    size_t length;
    float * xs;
    float * ys;
    SDL_Color * colors;
};

struct output_vertex;
struct output_vertex {
    struct output_vertex * next;
    float x;
    float y;
    float scale;
};

struct output_vertex * output_vertex_list_parse(const char * str);
const char * output_vertex_list_serialize(struct output_vertex * head); // not re-entrant!!!
void output_vertex_list_destroy(struct output_vertex * head);

//

struct output_device;
struct output_device {
    struct output_device * next;
    struct output_pixels pixels;
    struct output_vertex * vertex_head;
    bool active;

    SDL_Color ui_color;
    char * ui_name;
};

extern struct output_device * output_device_head;
extern unsigned int output_render_count;

// Calculate pixel coordinates from vertex coordinates
int output_device_arrange(struct output_device * dev);

// Render all of the output device pixel buffers
int output_render();
