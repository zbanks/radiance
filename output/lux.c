#include "util/config.h"
#include "util/string.h"
#include "util/err.h"
#include "util/math.h"
#include "output/lux.h"
#include "output/slice.h"
#include "output/config.h"

#define LUX_DEBUG INFO
#include "liblux/lux.h"

#define LUX_BROADCAST_ADDRESS 0xFFFFFFFF

enum lux_device_type {
    LUX_DEVICE_TYPE_STRIP,
    LUX_DEVICE_TYPE_SPOT,
    LUX_DEVICE_TYPE_GRID,
};

struct lux_channel;
struct lux_device;

struct lux_channel {
    int fd;
    bool sync;
    int id;
    struct lux_channel * next;
    struct lux_device * device_head;
};

struct lux_device {
    struct output_device base;

    struct lux_channel * channel;
    enum lux_device_type type;
    uint32_t address;
    char * descriptor;
    int length;
    size_t frame_buffer_size;
    uint8_t * frame_buffer;

    double max_energy;
    int oversample;
    double gamma;

    // Strip-only
    int strip_quantize;

    // Spot-only

    // Grid-only
    int grid_width;
    int grid_height;
};

static struct lux_channel * channel_head = NULL;
static struct lux_device * strip_devices = NULL;
static size_t n_strip_devices = 0;
static struct lux_device * spot_devices = NULL;
static size_t n_spot_devices = 0;
static struct lux_device * grid_devices = NULL;
static size_t n_grid_devices = 0;

//

static int lux_strip_get_length (int fd, uint32_t lux_id, int flags) {
    // TODO: replace with get_descriptor
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_GET_LENGTH,
        .index = 0,
        .payload_length = 0,
    };
    struct lux_packet response;
    int rc = lux_command(fd, &packet, &response, flags);
    if (rc < 0 || response.payload_length < 2) {
        ERROR("No/invalid response to length query on %#08x", lux_id);
        return -1;
    }

    uint16_t length;
    memcpy(&length, response.payload, sizeof length);

    INFO("Found strip on %#08x with length %d", lux_id, length);

    return length;
}

static int lux_strip_frame (int fd, uint32_t lux_id, unsigned char * data, size_t data_size) {
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_FRAME,
        .index = 0,
        .payload_length = data_size,
    };
    memcpy(packet.payload, data, data_size);
    LOGLIMIT(DEBUG, "Writing %ld bytes to %#08x", data_size, lux_id);
    return lux_write(fd, &packet, 0);
}

/*
static int lux_spot_frame (int fd, uint32_t lux_id, unsigned char * data, size_t data_size) {
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_FRAME,
        .index = 0,
        .payload_length = data_size,
    };
    memcpy(packet.payload, data, data_size);

    return lux_write(fd, &packet, 0);
}
*/

static int lux_grid_get_size (int fd, uint32_t lux_id, int flags, int * out_width, int * out_height) {
    // TODO: replace with get_descriptor
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_GET_LENGTH,
        .index = 0,
        .payload_length = 0,
    };
    struct lux_packet response;
    int rc = lux_command(fd, &packet, &response, flags);
    if (rc < 0 || response.payload_length < 6) {
        ERROR("No/invalid response to length query on %#08x", lux_id);
        return -1;
    }

    uint16_t total_length;
    uint16_t width;
    uint16_t height;
    memcpy(&total_length, &response.payload[0], 2);
    memcpy(&width, &response.payload[2], 2);
    memcpy(&height, &response.payload[4], 2);

    INFO("Found grid on %#08x with length %d and size %dx%d",
            lux_id, total_length, width, height);

    if (width * height != total_length)
        WARN("Width * Height != Length for grid: %d * %d != %d",
                width, height, total_length);

    *out_width = width;
    *out_height = height;
    return total_length;
}

static int (*lux_grid_frame) (int fd, uint32_t lux_id, unsigned char * data, size_t data_size)
    = lux_strip_frame;

static int lux_frame_sync (int fd, uint32_t lux_id) {
    return lux_sync(fd, 10);
    /*
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_SYNC,
        .index = 0,
        .payload_length = 0,
    };

    return lux_write(fd, &packet, 0);
    */
}

//

static struct lux_channel * lux_channel_create (const char * uri) {
    struct lux_channel * channel = calloc(1, sizeof *channel);
    if (channel == NULL) MEMFAIL();

    channel->fd = lux_uri_open(uri);
    if (channel->fd < 0) {
        PERROR("Unable to open lux socket '%s'", uri);
        free(channel);
        return NULL;
    }
    channel->id = -1;
    // Success!
    INFO("Initialized lux output channel '%s'", uri);
    channel->next = channel_head;
    channel_head = channel;
    return channel;
}

static void lux_channel_destroy_all() {
    struct lux_channel * channel = channel_head;
    while (channel != NULL) {
        lux_close(channel->fd);
        struct lux_channel * prev_channel = channel;
        channel = channel->next;
        free(prev_channel);
    }
    channel_head = NULL;
}

//
static uint8_t apply_gamma(double x, double gamma) {
    if (fabs(gamma - 1.) > 0.01)
        x = pow(x / 255., gamma) * 255.;
    return x;
}

static int lux_strip_prepare_frame(struct lux_device * device) {
    if (device->frame_buffer == NULL ||
        device->base.pixels.colors == NULL) return -1;

    uint8_t * frame_ptr = device->frame_buffer;
    SDL_Color * pixel_ptr = device->base.pixels.colors + device->base.pixels.length - 1;
#define PXL(x) apply_gamma((x) / (255. * device->oversample), device->gamma)

    if (device->strip_quantize > 0) {
        int l = 0; 
        for (int i = 0; i < device->strip_quantize; i++) {
            unsigned int r = 0, g = 0, b = 0; 
            for (int k = 0; k < device->oversample; k++) {
                r += pixel_ptr->r * pixel_ptr->a;
                g += pixel_ptr->g * pixel_ptr->a;
                b += pixel_ptr->b * pixel_ptr->a;
                pixel_ptr--;
            }
            while (l * device->strip_quantize < (i+1) * device->length) {
                *frame_ptr++ = PXL(r);
                *frame_ptr++ = PXL(g);
                *frame_ptr++ = PXL(b);
                l++;
            }
        }
    } else {
        for (int l = 0; l < device->length; l++) {
            unsigned int r = 0, g = 0, b = 0; 
            for (int k = 0; k < device->oversample; k++) {
                r += pixel_ptr->r * pixel_ptr->a;
                g += pixel_ptr->g * pixel_ptr->a;
                b += pixel_ptr->b * pixel_ptr->a;
                pixel_ptr--;
            }
            *frame_ptr++ = PXL(r);
            *frame_ptr++ = PXL(g);
            *frame_ptr++ = PXL(b);
        }
    }
#undef PXL

    double energy = 0;
    frame_ptr = device->frame_buffer;
    for (size_t l = 0; l < device->frame_buffer_size; l++)
        energy += *frame_ptr++;
    energy /= device->frame_buffer_size * 255; 
    if (energy > device->max_energy) {
        double ratio = device->max_energy / energy;
        frame_ptr = device->frame_buffer;
        for (size_t l = 0; l < device->frame_buffer_size; l++)
            *frame_ptr++ *= ratio;
    }
    return 0;
}

/*
static int lux_spot_prepare_frame(struct lux_device * device) {
    //TODO
    return 0;
}
*/

static int (*lux_grid_prepare_frame)(struct lux_device * device) = lux_strip_prepare_frame;

static void lux_device_term(struct lux_device * device) {
    //free(device->base.pixels.xs);
    //free(device->base.pixels.ys);
    //free(device->base.pixels.colors);
    //output_vertex_list_destroy(device->base.vertex_head);
    free(device->descriptor);
    free(device->frame_buffer);
    //free(device->ui_name);
    if (device->base.prev != NULL)
        device->base.prev->next = device->base.next;
    if (device->base.next != NULL)
        device->base.next->prev = device->base.prev;
    memset(device, 0, sizeof *device);
}

// 

void output_lux_term() {
    lux_channel_destroy_all();
    for (size_t i = 0; i < n_strip_devices; i++)
        lux_device_term(&strip_devices[i]);
    for (size_t i = 0; i < n_spot_devices; i++)
        lux_device_term(&spot_devices[i]);
    for (size_t i = 0; i < n_grid_devices; i++)
        lux_device_term(&grid_devices[i]);
    INFO("Lux terminated");
}

int output_lux_init() {
    // Set global configuration
    lux_timeout_ms = output_config.lux.timeout_ms;

    // Configure the channels
    for (int i = 0; i < output_config.n_lux_channels; i++) {
        if (!output_config.lux_channels[i].configured) continue;

        struct lux_channel * channel = lux_channel_create(output_config.lux_channels[i].uri);
        if (channel == NULL) continue;
        channel->sync = output_config.lux_channels[i].sync;
        channel->id = i;
    }

    // Initialize the devices, unconnected
    n_strip_devices = output_config.n_lux_strips;
    strip_devices = calloc(sizeof *strip_devices, n_strip_devices);
    if (strip_devices == NULL) MEMFAIL();

    n_spot_devices = output_config.n_lux_spots;
    spot_devices = calloc(sizeof *spot_devices, n_spot_devices);
    if (spot_devices == NULL) MEMFAIL();

    n_grid_devices = output_config.n_lux_grids;
    grid_devices = calloc(sizeof *grid_devices, n_grid_devices);
    if (grid_devices == NULL) MEMFAIL();

    // Search the open lux channels for the configured devices
    DEBUG("Starting lux device enumeration");
    int found_count = 0;
    for (size_t i = 0; i < n_strip_devices; i++) {
        struct lux_device * device = &strip_devices[i];
        memset(device, 0, sizeof *device);
        if (!output_config.lux_strips[i].configured)
            continue;
        if (output_device_head != NULL)
            output_device_head->prev = &device->base;
        device->base.next = output_device_head;
        output_device_head = &device->base;

        device->base.prev = NULL;

        device->base.vertex_head = output_config.lux_strips[i].vertexlist;
        device->base.active = false;
        device->base.ui_color = output_config.lux_strips[i].ui_color;
        device->base.ui_name = output_config.lux_strips[i].ui_name;

        device->address  = output_config.lux_strips[i].address;
        device->max_energy = CLAMP(output_config.lux_strips[i].max_energy, 0, 1);
        device->oversample = MAX(1, output_config.lux_strips[i].oversample);
        device->gamma = output_config.lux_strips[i].gamma;
        device->strip_quantize = output_config.lux_strips[i].quantize;

        if (output_config.lux_strips[i].channel >= 0 && output_config.lux_strips[i].length >= 0) {
            device->length = output_config.lux_strips[i].length;
            for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
                if (channel->id == output_config.lux_strips[i].channel) {
                    device->channel = channel;
                    device->base.active = true;
                    found_count++;
                    DEBUG("Using hardcoded channel for strip %zu", i);
                    break;
                }
            }
        } else {
            device->length = -1;
            for (int i = 0; i < 2; i++) { // Try twice to find the strips
                for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
                    int length = lux_strip_get_length(channel->fd, device->address, 0);
                    if (length < 0)
                        continue;

                    device->length = length;
                    device->channel = channel;
                    device->base.active = true;
                    found_count++;
                    goto found_strip;
                }
            }
            found_strip:;
        }

        if (device->base.active) {
            device->frame_buffer_size = device->length * 3;
            if (device->strip_quantize > 0) {
                device->base.pixels.length = device->oversample * device->strip_quantize;
            } else {
                device->base.pixels.length = device->oversample * device->length;
            }

            device->frame_buffer = calloc(1, device->frame_buffer_size);
            if (device->frame_buffer == NULL) MEMFAIL();

            output_device_arrange(&device->base);
        }
    }
    /*
    for (size_t i = 0; i < n_spot_devices; i++) {
        struct lux_device * device = &spot_devices[i];
        memset(device, 0, sizeof *device);
        if (!output_config.lux_spots[i].configured)
            continue;
        if (output_device_head != NULL)
            output_device_head->prev = &device->base;
        device->base.next = output_device_head;
        device->base.prev = NULL;
        output_device_head = &device->base;

        device->base.vertex_head = output_config.lux_spots[i].vertexlist;
        device->base.active = false;
        device->base.ui_color = output_config.lux_spots[i].ui_color;
        device->base.ui_name = output_config.lux_spots[i].ui_name;

        device->address  = output_config.lux_spots[i].address;
        device->max_energy = output_config.lux_spots[i].max_energy;
        device->oversample = output_config.lux_spots[i].oversample;

        for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
            continue;

            device->channel = channel;
            device->base.active = false;
            found_count++;
            break;
        }
        device->base.pixels.length = device->oversample;
        output_device_arrange(&device->base);
    }
    */
    for (size_t i = 0; i < n_grid_devices; i++) {
        struct lux_device * device = &grid_devices[i];
        memset(device, 0, sizeof *device);
        if (!output_config.lux_grids[i].configured)
            continue;
        if (output_device_head != NULL)
            output_device_head->prev = &device->base;
        device->base.next = output_device_head;
        output_device_head = &device->base;

        device->base.prev = NULL;

        device->base.vertex_head = output_config.lux_grids[i].vertexlist;
        device->base.active = false;
        device->base.ui_color = output_config.lux_grids[i].ui_color;
        device->base.ui_name = output_config.lux_grids[i].ui_name;

        device->address  = output_config.lux_grids[i].address;
        device->max_energy = CLAMP(output_config.lux_grids[i].max_energy, 0, 1);
        device->oversample = 1; //MAX(1, output_config.lux_grids[i].oversample);
        device->gamma = output_config.lux_grids[i].gamma;

        if (output_config.lux_grids[i].channel >= 0
         && output_config.lux_grids[i].width >= 0
         && output_config.lux_grids[i].height >= 0) {
            device->grid_width = output_config.lux_grids[i].width;
            device->grid_height = output_config.lux_grids[i].height;
            device->length = device->grid_width * device->grid_height;
            for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
                if (channel->id == output_config.lux_grids[i].channel) {
                    device->channel = channel;
                    device->base.active = true;
                    found_count++;
                    DEBUG("Using hardcoded channel for grid %zu", i);
                    break;
                }
            }
        } else {
            device->grid_width = -1;
            device->grid_height = -1;
            for (int i = 0; i < 2; i++) { // Try twice to find the strips
                for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
                    int length = lux_grid_get_size(
                            channel->fd, device->address, 0,
                            &device->grid_width, &device->grid_height);
                    if (length < 0)
                        continue;

                    device->length = length;
                    device->channel = channel;
                    device->base.active = true;
                    found_count++;
                    goto found_grid;
                }
            }
            found_grid:;
        }

        if (device->base.active) {
            device->frame_buffer_size = device->length * 3;
            // TODO: Implement oversample; quantize
            device->base.pixels.length = device->length;

            device->frame_buffer = calloc(1, device->frame_buffer_size);
            if (device->frame_buffer == NULL) MEMFAIL();

            int rc = output_device_arrange_grid(&device->base, device->grid_width, device->grid_height);
            if (rc < 0)
                ERROR("Unable to arrange pixels for grid %zu", i);
        }
    }

    INFO("Finished lux enumeration and found %d/%lu devices",
         found_count, n_strip_devices + n_spot_devices);
    INFO("Lux initialized");
    return 0;
}

int output_lux_prepare_frame() {
    int rc = 0; (void) rc;
    for (size_t i = 0; i < n_strip_devices; i++) {
        struct lux_device * device = &strip_devices[i];
        if (device->channel == NULL) continue;
        rc = lux_strip_prepare_frame(device);
        if (rc < 0) continue;
        rc = lux_strip_frame(
                device->channel->fd,
                device->address,
                device->frame_buffer,
                device->frame_buffer_size);
        if (rc < 0) LOGLIMIT(WARN, "Unable to send frame to %#08x", device->address);
    }
    /*
    for (size_t i = 0; i < n_spot_devices; i++) {
        struct lux_device * device = &spot_devices[i];
        if (device->channel == NULL) continue;
        rc = lux_spot_prepare_frame(device);
        if (rc < 0) continue;
        rc = lux_spot_frame(
                device->channel->fd,
                device->address,
                device->frame_buffer,
                device->frame_buffer_size);
        if (rc < 0) LOGLIMIT(WARN("Unable to send frame to %#08x", device->address));
    }
    */
    for (size_t i = 0; i < n_grid_devices; i++) {
        struct lux_device * device = &grid_devices[i];
        if (device->channel == NULL) continue;
        rc = lux_grid_prepare_frame(device);
        if (rc < 0) continue;
        rc = lux_grid_frame(
                device->channel->fd,
                device->address,
                device->frame_buffer,
                device->frame_buffer_size);
        if (rc < 0) LOGLIMIT(WARN, "Unable to send frame to %#08x", device->address);
    }
    return 0;
}

int output_lux_sync_frame() {
    for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
        if (!channel->sync) continue;
        int rc = lux_frame_sync(channel->fd, LUX_BROADCAST_ADDRESS);
        if (rc < 0) LOGLIMIT(WARN, "Unable to send sync message on fd %d", channel->fd);
    }
    return 0;
}
