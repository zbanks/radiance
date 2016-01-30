#include "core/config.h"
#include "core/err.h"
#include "output/lux.h"
#include "output/slice.h"

#include "output/lux_wire.h"

#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>

static int lux_socket = -1;
static fd_set lux_fds;
static int lux_n_fds;

static int lux_length(uint32_t lux_id, int * output_length) {
    unsigned char frame[LUX_PACKET_MAX_SIZE];
    size_t frame_len = 0;

    memcpy(frame, &lux_id, 4);
    frame[4] = CMD_GET_LENGTH;
    frame_len = 5;

    send(lux_socket, frame, frame_len, 0);

    struct timeval timeout = {
        .tv_sec = config.lux.timeout / 1000,
        .tv_usec = (config.lux.timeout % 1000) * 1000,
    };

    int rv = select(lux_n_fds, &lux_fds, NULL, NULL, &timeout);
    //printf("select returned %d\n", rv);
    if (rv < 0) {
        printf("select() error\n");
        return -1;
    } else if (rv >= 1) {
        rv = recv(lux_socket, frame, LUX_PACKET_MAX_SIZE, 0);
        //printf("recieved n bytes %d\n", rv);
        if (rv == -1) return -1;

        uint16_t length;
        memcpy(&length, &frame[4], 2);
        *output_length = length;
    } else {
        printf("No response to length query on %#X\n", lux_id);
        return -1;
    }

    return 0;
}

static int lux_frame(uint32_t lux_id, unsigned char * data, size_t data_size) {
    unsigned char frame[LUX_PACKET_MAX_SIZE];
    size_t frame_len = 0;

    memcpy(frame, &lux_id, 4);
    frame[4] = CMD_FRAME;
    memcpy(&frame[5], data, data_size);
    frame_len = 5 + data_size;

    send(lux_socket, frame, frame_len, 0);
    return 0;
}

int output_lux_init() {
    if (lux_socket >= 0) {
        // Already connected
        return -1;
    }

    lux_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(config.lux.address);
    addr.sin_port = htons(config.lux.port);
    int rc = connect(lux_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (rc != 0) {
        printf("ERROR connecting to socket\n");
        return -1;
    }

    FD_ZERO(&lux_fds);
    FD_SET(lux_socket, &lux_fds);
    lux_n_fds = lux_socket + 1;

    printf("Lux initialized\n");

    return 0;
}

void output_lux_del() {
    if (lux_socket >= 0) {
        shutdown(lux_socket, 0);
    }
}

int output_lux_enumerate(output_strip_t * strips, int n_strips) {
    int found = 0;
    for (int i = 0; i < n_strips; i++) {
        int length;
        if (lux_length(strips[i].id_int, &length) != 0) {
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

