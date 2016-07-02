#include "util/config.h"
#include "util/string.h"
#include "util/err.h"
#include "output/lux.h"
#include "output/slice.h"
//#include "output/config.h"

#include "liblux/lux.h"

#define LUX_BROADCAST_ADDRESS 0xFFFFFFFF

enum lux_device_type {
    LUX_DEVICE_TYPE_STRIP,
    LUX_DEVICE_TYPE_SPOT,
};

struct lux_channel;
struct lux_device;

struct lux_channel {
    int fd;
    struct lux_channel * next;
    struct lux_device * device_head;
};

struct lux_device {
    struct output_device base;
    struct lux_device * next;

    enum lux_device_type type;
    uint32_t address;
    char * descriptor;
    size_t frame_buffer_size;
    uint8_t * frame_buffer;

    double max_energy;
    int oversample;

    // Strip-only
    int strip_length;
    int strip_quantize;

    // Spot-only
};

static struct lux_channel * channel_head = NULL;

//

static int lux_strip_get_length (int fd, uint32_t lux_id) {
    // TODO: replace with get_descriptor
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_GET_LENGTH,
        .index = 0,
        .payload_length = 0,
    };
    struct lux_packet response;
    int rc = lux_command(fd, &packet, &response, LUX_RETRY);
    if (rc < 0 || response.payload_length != 2) {
        printf("No/invalid response to length query on %#X\n", lux_id);
        return -1;
    }

    uint16_t length;
    memcpy(&length, response.payload, sizeof length);

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

    return lux_write(fd, &packet, 0);
}

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

static int lux_frame_sync (int fd, uint32_t lux_id) {
    struct lux_packet packet = {
        .destination = lux_id,
        .command = LUX_CMD_SYNC,
        .index = 0,
        .payload_length = 0,
    };

    return lux_write(fd, &packet, 0);
}

//

static struct lux_channel * lux_channel_create (const char * uri) {
    struct lux_channel * channel = calloc(1, sizeof *channel);
    if (channel == NULL) MEMFAIL();

    char * portstr = strdup(uri);
    if (portstr == NULL) MEMFAIL();
    char * address = strsep(&portstr, ":");
    if (portstr == NULL || portstr[0] == '\0' || address[0] == '\0') {
        ERROR("Invalid lux uri '%s'; expected 'ip:port'", uri);
        errno = EINVAL;
        goto fail;
    }
    int port = atoi(portstr);
    if (port <= 0) {
        ERROR("Invalid lux port '%s'", portstr);
        errno = EINVAL;
        goto fail;
    }

    channel->fd = lux_network_open(address, port);
    if (channel->fd < 0) {
        PERROR("Unable to open lux socket");
        goto fail;
    }
    free(address);
    
    // Success!
    channel->next = channel_head;
    channel_head = channel;
    return channel;

fail:
    free(channel);
    free(address);
    return NULL;
}

static void lux_channel_destroy_all() {
    struct lux_channel * channel = channel_head;
    while (channel != NULL) {
        struct lux_device * device= channel->device_head;
        //TODO: Destroy devices
        (void) (device);

        lux_close(channel->fd);
        struct lux_channel * prev_channel = channel;
        channel = channel->next;
        free(prev_channel);
    }
    channel_head = NULL;
}

//

static int lux_strip_setup(struct lux_device * device) {
    //TODO
    return 0;
}

static int lux_strip_prepare_frame(struct lux_device * device) {
    //TODO
    return 0;
}

static int lux_spot_setup(struct lux_device * device) {
    //TODO
    return 0;
}

static int lux_spot_prepare_frame(struct lux_device * device) {
    //TODO
    return 0;
}

// 

int output_lux_init() {
    if (channel_head != NULL) {
        // Already connected
        ERROR("Cannot init lux; already inited");
        return -1;
    }

    lux_timeout_ms = config.lux.timeout_ms;
    struct lux_channel * channel = lux_channel_create(config.lux.uri);
    if (channel == NULL) return -1;

    int rc = output_lux_enumerate();
    if (rc < 0) return rc;

    INFO("Lux initialized");
    return 0;
}

void output_lux_term() {
    lux_channel_destroy_all();
    INFO("Lux terminated");
}

int output_lux_enumerate() {
    int found = 0;
    for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
        for (struct lux_device * device = channel->device_head; device; device = device->next) {
            device->base.active = false;

            int rc, length;
            switch (device->type) {
            case LUX_DEVICE_TYPE_STRIP:
                length = lux_strip_get_length(channel->fd, device->address);
                if (length < 0) {
                    INFO("No response from device %#08x", device->address);
                    goto device_fail;
                }

                device->strip_length = length;
                rc = lux_strip_setup(device);
                if (rc < 0) {
                    WARN("Unable to set up lux device %#08x (length %d)", device->address, length);
                    goto device_fail;
                }
                break;

            case LUX_DEVICE_TYPE_SPOT:;
                ERROR("Can't enumerate spot devices yet"); // TODO

                rc = lux_spot_setup(device);
                if (rc < 0) {
                    WARN("Unable to set up lux device %#08x (length %d)", device->address, length);
                    goto device_fail;
                }
                break;

            default:
                ERROR("Unknown device type %d", device->type);
                goto device_fail;
            }

            device->base.active = true;
            found++;
            continue;
device_fail:;
        }
    }
    return found;
}

int output_lux_prepare_frame() {
    int rc = 0; (void) rc;
    for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
        for (struct lux_device * device = channel->device_head; device; device = device->next) {
            if (!device->base.active) continue;

            switch (device->type) {
            case LUX_DEVICE_TYPE_STRIP:
                rc = lux_strip_prepare_frame(device);
                rc = lux_strip_frame(
                        channel->fd,
                        device->address,
                        device->frame_buffer,
                        device->frame_buffer_size);
                break;
            case LUX_DEVICE_TYPE_SPOT:
                rc = lux_spot_prepare_frame(device);
                rc = lux_spot_frame(
                        channel->fd,
                        device->address,
                        device->frame_buffer,
                        device->frame_buffer_size);
                break;
            default:
                WARN("Invalid lux device '%u'; skipping", device->type);
                break;
            }
        }
    }
    return 0;
}

int output_lux_sync_frame() {
    for (struct lux_channel * channel = channel_head; channel; channel = channel->next) {
        int rc = lux_frame_sync(channel->fd, LUX_BROADCAST_ADDRESS);
        if (rc < 0) LOGLIMIT(WARN("Unable to send sync message on fd"));
    }
    return 0;
}
