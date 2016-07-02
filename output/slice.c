#include "output/slice.h"
#include "util/math.h"

struct output_device * output_device_head = NULL;
unsigned int output_render_count = 0;

int output_device_arrange(struct output_device * dev) {
    size_t length = dev->pixels.length;
    if (length <= 0) return -1;
    if (dev->vertex_head == NULL) return -1;

    if (dev->vertex_head->next == NULL) {
        // Special case - only a single point
        for (size_t i = 0; i < length; i++) {
            dev->pixels.xs[i] = dev->vertex_head->x;
            dev->pixels.ys[i] = dev->vertex_head->y;
        }
        return 0;
    }

    // Sum up scales of all the verticies except the last
    double scale_sum = 0.;
    for (const struct output_vertex * vert = dev->vertex_head; vert->next; vert = vert->next)
        scale_sum += vert->scale * hypot(vert->x - vert->next->x, vert->y - vert->next->y);

    // Interpolate pixel values
    double scale_per_pixel = scale_sum / (double) length;
    double cumulative_scale = 0.;
    size_t pixel_idx = 0;
    for (const struct output_vertex * vert = dev->vertex_head; vert->next; vert = vert->next) {
        double vert_scale = vert->scale * hypot(vert->x - vert->next->x, vert->y - vert->next->y);
        while (pixel_idx * scale_per_pixel <= cumulative_scale + vert_scale) {
            double alpha = (pixel_idx * scale_per_pixel - cumulative_scale) / vert_scale;
            dev->pixels.xs[pixel_idx] = INTERP(alpha, vert->x, vert->next->x);
            dev->pixels.ys[pixel_idx] = INTERP(alpha, vert->y, vert->next->y);
            pixel_idx++;
        }
        cumulative_scale += vert_scale;
    }

    return 0;
}

int output_render() {
    for (struct output_device * dev = output_device_head; dev; dev = dev->next) {
        if (!dev->active) continue;

        int rc = 0; //TODO: __todo_render(&dev->pixels);
        if (rc < 0) return rc;
    }
    output_render_count++;
    return 0;
}

