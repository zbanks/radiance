#include "liblux/lux.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define _ERR_STRINGIFY2(x) #x
#define _ERR_STRINGIFY(x) _ERR_STRINGIFY2(x)

#define DEBUG_INFO __FILE__ ":" _ERR_STRINGIFY(__LINE__) ":" _ERR_STRINGIFY(__func__)
#define _ERR_MSG(severity, msg, ...) fprintf(stderr, "[%-5s] [%s:%s:%d] " msg "\n", _ERR_STRINGIFY(severity), __FILE__, __func__, __LINE__, ## __VA_ARGS__);

#define FAIL(...) ({ERROR(__VA_ARGS__); exit(EXIT_FAILURE);})
#define ERROR(...) _ERR_MSG(ERROR, ## __VA_ARGS__)
#define WARN(...)  _ERR_MSG(WARN,  ## __VA_ARGS__)
#define INFO(...)  _ERR_MSG(INFO,  ## __VA_ARGS__)
#define DEBUG(...) _ERR_MSG(DEBUG, ## __VA_ARGS__)
#define MEMFAIL() PFAIL("Could not allocate memory")

#define PFAIL(...) ({ERROR(__VA_ARGS__); exit(EXIT_FAILURE);})
#define PERROR(msg, ...) _ERR_MSG(ERROR,"[%s] " msg, strerror(errno), ## __VA_ARGS__)

static uint32_t ADDRESS = 0x12;
int loglevel = 0;

static int check_packet_count(int fd) {
    struct lux_packet response;
    struct lux_packet packet2 = {
        .destination = ADDRESS, 
        .command = LUX_CMD_GET_PKTCNT,
        .index = 0,
        .payload_length = 0,
    };
    int rc = lux_command(fd, &packet2, &response, LUX_RETRY);
    if (rc < 0) {
        PERROR("Packet check count failed");
        return -1;
    }

    printf("Received packet for %#08x; cmd=%#02x; idx=%d; plen=%d; data=",
            response.destination, response.command, response.index, response.payload_length);
    uint32_t *x = (uint32_t *) response.payload;
    /*
    uint32_t good_packet;
    uint32_t malformed_packet;
    uint32_t packet_overrun;
    uint32_t bad_checksum;
    uint32_t rx_interrupted;
    */
    printf("good:%d malfm:%d ovrun:%d badcrc:%d rxint:%d xaddr:%d\n", x[0], x[1], x[2], x[3], x[4], x[5]);
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
        int rc = lux_command(fd, &packet, &response, LUX_ACK | LUX_RETRY);
        if (rc < 0) PERROR("Unable to write LED on msg");
        usleep(100000);

        packet.payload[0] = 0;
        rc = lux_command(fd, &packet, &response, LUX_ACK | LUX_RETRY);
        if (rc < 0) PERROR("Unable to write LED off msg");
        usleep(100000);
    }

    return 0;
}

int main(void) {
    //int fd = lux_uri_open("serial:///dev/ttyACM0");
    int fd = lux_uri_open("udp://127.0.0.1:1365");
    check_packet_count(fd);
    blink_led(fd, 0xFFFFFFFF, 10);
    return 0;
}
