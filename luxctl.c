#define _BSD_SOURCE // for usleep

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "liblux/lux.h"
#include "util/err.h"

enum loglevel loglevel = LOGLEVEL_INFO;

static int send_test_messages(int fd, uint32_t addr, size_t count, long usec) {
    struct timeval t1, t2;

    // ---- 
    struct lux_packet packet = {
        .destination = addr,
        .command = LUX_CMD_FRAME,
        .index = 0,
        .payload_length = 300 * 3,
    };

    gettimeofday(&t1, NULL);
    count |= 0xFF;
    for (size_t i = 0; i < count; i++) {
        //packet.destination = i % 10;
        //memset(&packet.payload, i & 0xFF, packet.payload_length);
        for (size_t k = 0; k < packet.payload_length / 3; k++) {
            packet.payload[k * 3 + 0] = i & 0x3F;
            packet.payload[k * 3 + 1] = i & 0x3F;
            packet.payload[k * 3 + 2] = i & 0x3F;
        }
        int rc = lux_write(fd, &packet, 0);
        if (rc < 0) {
            PERROR("Unable to write");
            return -1;
        }
        usleep(usec);
    }
    // ---- 
    gettimeofday(&t2, NULL);

    long delta = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
    double ratio = 0.001 * ((double) delta) / ((double) count);
    INFO("Time delta: %ld micros for %ld writes", delta, count);
    INFO("Avg: %0.3lf ms/write; %0.3lf ms delay/write", ratio, 0.001 * (double) usec);

    return 0;
}

static int send_test_commands(int fd, uint32_t addr, int count) {
    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    // ---- 
    struct lux_packet packet = {
        .destination = addr, 
        .command = LUX_CMD_GET_ID,
        .index = 0,
        .payload_length = 0,
    };

    struct lux_packet response;

    int rc = 0;
    for (int i = 0; i < count; i++)
        rc |= lux_command(fd, &packet, &response, LUX_RETRY);
    // ---- 
    gettimeofday(&t2, NULL);


    long delta = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
    double ratio = 0.001 * ((double) delta) / ((double) count);
    INFO("Time delta: %ld micros for %d commands", delta, count);
    INFO("Avg: %0.3lf ms/write", ratio);

    if (rc < 0) {
        PERROR("Error in a command");
        return -1;
    }

    INFO("Received packet for %#08x; cmd=%#02x; idx=%d; plen=%d; data=",
            response.destination, response.command, response.index, response.payload_length);
    INFO("'%.*s'", response.payload_length, response.payload);
    return 0;
}

static int reset_packet_count(int fd, int addr) {
    struct lux_packet response;
    struct lux_packet packet2 = {
        .destination = addr, 
        .command = LUX_CMD_RESET_PKTCNT,
        .index = 0,
        .payload_length = 0,
    };
    int rc = lux_command(fd, &packet2, &response, LUX_RETRY | LUX_ACK);
    if (rc < 0) {
        PERROR("Unable to send reset command to %#08x", addr);
        return -1;
    }

    INFO("Reset packet counts on %#08x", addr);
    return 0;
}

static int commit_config(int fd, int addr) {
    // This is not needed for most firmware
    struct lux_packet response;
    struct lux_packet packet = {
        .command = LUX_CMD_RESET_PKTCNT,
        .destination = addr, 
        .index = 0,
        .payload_length = 0,
    };
    int rc = lux_command(fd, &packet, &response, LUX_RETRY | LUX_ACK);
    if (rc < 0) {
        PERROR("Unable to send commit config command to %#08x", addr);
        return -1;
    }

    INFO("Committed config on %#08x", addr);
    return 0;
}

static int blink_led(int fd, uint32_t addr, int count) {
    struct lux_packet packet = {
        .destination = addr, 
        .command = LUX_CMD_SET_LED,
        .index = 0,
        .payload_length = 1,
    };
    struct lux_packet response;

    while (count--) {
        packet.payload[0] = 1;
        int rc = lux_command(fd, &packet, &response, LUX_ACK);
        if (rc < 0) PERROR("Unable to turn on  LED for %#08x", addr);
        usleep(100000);

        packet.payload[0] = 0;
        rc = lux_command(fd, &packet, &response, LUX_ACK);
        if (rc < 0) PERROR("Unable to turn off LED for %#08x", addr);
        usleep(100000);
    }

    return 0;
}

static int check_packet_count(int fd, int addr) {
    struct lux_packet response;
    struct lux_packet packet2 = {
        .destination = addr, 
        .command = LUX_CMD_GET_PKTCNT,
        .index = 0,
        .payload_length = 0,
    };
    int rc = lux_command(fd, &packet2, &response, LUX_RETRY);
    if (rc < 0) {
        PERROR("Packet check count failed");
        return -1;
    }

    INFO("Received packet for %#08x; cmd=%#02x; idx=%d; plen=%d;",
            response.destination, response.command, response.index, response.payload_length);
    uint32_t *x = (uint32_t *) response.payload;
    /* Response payload:
    uint32_t good_packet;
    uint32_t malformed_packet;
    uint32_t packet_overrun;
    uint32_t bad_checksum;
    uint32_t rx_interrupted;
    uint32_t wrong_address;
    */
    INFO("Counters: good=%d malfm=%d ovrun=%d badcrc=%d rxint=%d xaddr=%d", x[0], x[1], x[2], x[3], x[4], x[5]);

    return 0;
}

static int assign_address(int fd, uint32_t old_addr, uint32_t new_addr) {
    struct lux_packet response;
    struct lux_packet command = {
        .destination = old_addr,
        .command = LUX_CMD_GET_ADDR
    };
    int rc = lux_command(fd, &command, &response, LUX_RETRY);
    if (rc < 0) {
        PERROR("Unable to get addresses");
        return -1;
    }

    command.command = LUX_CMD_SET_ADDR;
    command.payload_length = response.payload_length;
    memcpy(command.payload, response.payload, response.payload_length);
    memcpy(&command.payload[8], &new_addr, sizeof new_addr);

    rc = lux_command(fd, &command, &response, LUX_ACK | LUX_RETRY);
    if (rc < 0) {
        PERROR("Unable to set addresses");
        return -1;
    }
    INFO("Reassigned address %#08x to %#08x", old_addr, new_addr);
    return 0;
}

static int set_length(int fd, int addr, uint16_t len) {
    struct lux_packet response;
    struct lux_packet command = {
        .destination = addr,
        .command = LUX_CMD_SET_LENGTH,
        .payload_length = 2,
    };
    memcpy(command.payload, &len, sizeof len);
    int rc = lux_command(fd, &command, &response, LUX_ACK | LUX_RETRY);
    if (rc < 0) {
        PERROR("Unable to set len");
        return -1;
    }
    INFO("Set length of address %#08x to %hu", addr, len);
    return 0;
}

static int usage() {
    fprintf(stderr, "\n\
  Usage: luxctl <lux_uri> <commands...>\n\
    Commands are executed serially, in order.\n\
    Flags specify commands, and can be used multiple times\n\
 \n\
  Lux URIs:\n\
    serial:///dev/ttyACM0\n\
    udp://127.0.0.1:1365\n\
 \n\
  Commands:\n\
    -a <address>        Use address for subsequent commands\n\
    -A <address>        Change the address of the device and\n\
                        use the new address for subsequent commands\n\
    -f <n>              Flood n FRAME packets \n\
    -i <n>              Flood n GET_ID commands \n\
    -b <n>              Blink the LED n times\n\
    -s                  Read packet statistics\n\
    -S                  Reset packet statistics\n\
    -L <len>            Set strip length\n\
    -C                  Commit config (legacy; do not use)\n\
");
    return 1;
}

int main(int argc, char ** argv) {
    if (argc < 2)
        return usage();

    int fd = lux_uri_open(argv[1]);
    if (fd < 0) {
        PERROR("Unable to open Lux URI '%s'", argv[1]);
        return 1;
    }

    uint32_t address = 0x80000000;
    optind = 2;
    int opt = -1;
    int rc = 0;
    while ((opt = getopt(argc, argv, "a:A:f:i:b:sSL:Ch")) != -1) {
        uint32_t loptarg = strtoul(optarg, NULL, 0);
        switch (opt) {
        case 'a': // use address
            address = loptarg;
            INFO("Using address %#08x", address);
            break;
        case 'A': // set address
            rc = assign_address(fd, address, loptarg);
            address = loptarg;
            break;
        case 'f': // flood frame packets
            rc = send_test_messages(fd, address, loptarg, 0);
            break;
        case 'i': // flood id commands
            rc = send_test_commands(fd, address, loptarg);
            break;
        case 'b': // blink LED
            rc = blink_led(fd, address, loptarg);
            break;
        case 's': // read stats
            rc = check_packet_count(fd, address);
            break;
        case 'S': // clear stats
            rc = reset_packet_count(fd, address);
            break;
        case 'L': // set length
            rc = set_length(fd, address, loptarg);
            break;
        case 'C':; // commit config
            rc = commit_config(fd, address);
            break;
        case 'h':; // usage / help
        default:
            return usage();
        }
        if (rc < 0) {
            ERROR("Command failed; quitting");
            break;
        }
    }

    lux_close(fd);
    return 0;
}

