#include "core/config.h"
#include "core/err.h"
#include "output/lux.h"
#include "output/slice.h"

#include "linux/lux.h"

#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>

static int lux_fd = -1;

static int lux_length(uint32_t lux_id) {
    struct lux_packet packet = {
        .destination = lux_id,
        .command = CMD_GET_LENGTH,
        .index = 0,
        .payload_length = 0,
    };
    struct lux_packet response;
    int rc = lux_command(lux_fd, &packet, 1, &response);
    if (rc < 0 || response.payload_length != 2) {
        printf("No/invalid response to length query on %#X\n", lux_id);
        return -1;
    }

    uint16_t length;
    memcpy(&length, response.payload, sizeof length);

    return length;
}

static int lux_frame(uint32_t lux_id, unsigned char * data, size_t data_size) {
    struct lux_packet packet = {
        .destination = lux_id,
        .command = CMD_FRAME,
        .index = 0,
        .payload_length = data_size,
    };
    memcpy(packet.payload, data, data_size);

    return lux_write(lux_fd, &packet);
}

int output_lux_init() {
    if (lux_fd >= 0) {
        // Already connected
        return -1;
    }

    lux_fd = lux_network_open(config.lux.address, config.lux.port);

    if (lux_fd < 0) {
        printf("ERROR connecting to socket\n");
        return -1;
    }

    printf("Lux initialized\n");

    return 0;
}

void output_lux_del() {
    if (lux_fd >= 0) {
        lux_close(lux_fd);
    }
}

int output_lux_enumerate(output_strip_t * strips, int n_strips) {
    int found = 0;
    for (int i = 0; i < n_strips; i++) {
        int length = lux_length(strips[i].id_int);
        if (length < 0) {
            strips[n_strips].bus &= ~OUTPUT_LUX;
            printf("Lux UDP device '%#X' not found\n", strips[i].id_int);
            continue;
        }
        strips[i].bus |= OUTPUT_LUX;
        strips[i].length = length;
        found++;

        printf("Lux UDP device '%#X' with length '%i'\n", strips[i].id_int, length);

    }
    return found;
}

int output_lux_push(output_strip_t * strip, unsigned char * frame, int length) {
    lux_frame(strip->id_int, frame, length);
    return 0;
}

