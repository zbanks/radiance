#ifndef __LINUX_LUX_H__
#define __LINUX_LUX_H__

#include <stdint.h>
#include "lux_cmds.h"

int lux_serial_open();
int lux_network_open(const char * address_spec, uint16_t port);
void lux_close(int fd);

struct lux_packet {
    uint32_t destination;
    enum lux_command command;
    uint8_t index;
    uint8_t payload[LUX_PACKET_MAX_SIZE];
    uint16_t payload_length;
    uint32_t crc;
};

int lux_write(int fd, struct lux_packet * packet);
int lux_command(int fd, struct lux_packet * packet, int retry, struct lux_packet * response);
int lux_command_ack(int fd, struct lux_packet * packet, int retry);

#endif
