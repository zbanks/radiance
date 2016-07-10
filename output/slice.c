#include "output/slice.h"
#include "util/err.h"
#include "util/math.h"
#include "util/string.h"
#include "ui/render.h"
#include "main.h" // FIXME

struct output_vertex * output_vertex_list_parse(const char * _str) {
    if (_str == NULL) return NULL;
    char * str = strdup(_str);
    if (str == NULL) MEMFAIL();

    struct output_vertex * vertex_head = NULL;
    char * vstr;
    while ((vstr = strsep(&str, ",")) != NULL) {
        float x, y, scale = 1.0;
        if ((sscanf(vstr, "%f %f %F", &x, &y, &scale)) < 2) {
            ERROR("Unable to parse term '%s'", vstr);
            goto fail;
        }
        struct output_vertex * new_vertex = calloc(1, sizeof *new_vertex);
        *new_vertex = (struct output_vertex) {
            .next = vertex_head,
            .x = x, .y = y,
            .scale = scale,
        };
        vertex_head = new_vertex;
    }
    return vertex_head;

fail:
    ERROR("Unable to parse vertex list '%s'", _str);
    output_vertex_list_destroy(vertex_head);
    return NULL;
}

const char * output_vertex_list_serialize(struct output_vertex * head) {
    static char * buf = NULL;
    static size_t buflen = 0;
    static const char * format = "%0.3f %0.3f %0.2f,";

    size_t len_needed = 1;
    for (struct output_vertex * v = head; v; v = v->next)
        len_needed += snprintf(NULL, 0, format, v->x, v->y, v->scale);

    if (buflen < len_needed || buf == NULL) {
        buf = realloc(buf, len_needed);
        buflen = len_needed;
        if (buf == NULL) MEMFAIL();
    }

    size_t len = 0;
    for (struct output_vertex * v = head; v; v = v->next)
        len += sprintf(buf + len, format, v->x, v->y, v->scale);
    if (len >= buflen) FAIL("Vertex list grew as writing (%lu > %lu)", len, buflen);
    

    buf[len-1] = '\0'; // Overwrite last ','
    return buf;
}

void output_vertex_list_destroy(struct output_vertex * head) {
    while (head != NULL) {
        struct output_vertex * v = head->next;
        free(head);
        head = v;
    }
}

//

struct output_device * output_device_head = NULL;
unsigned int output_render_count = 0;

int output_device_arrange(struct output_device * dev) {
    size_t length = dev->pixels.length;
    if (length <= 0) return -1;
    if (dev->vertex_head == NULL) return -1;

    // Realloc pixel arrays
    dev->pixels.xs = realloc(dev->pixels.xs, length * sizeof *dev->pixels.xs);
    dev->pixels.ys = realloc(dev->pixels.ys, length * sizeof *dev->pixels.ys);
    dev->pixels.colors = realloc(dev->pixels.colors, length * sizeof *dev->pixels.colors);
    if (dev->pixels.xs == NULL || dev->pixels.ys == NULL || dev->pixels.colors == NULL) MEMFAIL();
    memset(dev->pixels.xs, 0, length * sizeof *dev->pixels.xs);
    memset(dev->pixels.ys, 0, length * sizeof *dev->pixels.ys);
    memset(dev->pixels.colors, 0, length * sizeof *dev->pixels.colors);

    // Special case - only a single point
    if (dev->vertex_head->next == NULL) {
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
            if (pixel_idx >= length) break;
            double alpha = (pixel_idx * scale_per_pixel - cumulative_scale) / vert_scale;
            dev->pixels.xs[pixel_idx] = INTERP(alpha, vert->x, vert->next->x);
            dev->pixels.ys[pixel_idx] = INTERP(alpha, vert->y, vert->next->y);
            pixel_idx++;
        }
        cumulative_scale += vert_scale;
    }

    return 0;
}

int output_render(struct render * render) {
    render_freeze(render);
    for (struct output_device * dev = output_device_head; dev; dev = dev->next) {
        if (!dev->active) continue;
        for (size_t i = 0; i < dev->pixels.length; i++)
            dev->pixels.colors[i] = render_sample(render, dev->pixels.xs[i], dev->pixels.ys[i]);
    }
    render_thaw(render);
    output_render_count++;
    return 0;
}

