#pragma once

#include <stdint.h>
#include "liblux/lux_cmds.h"

struct lux_packet {
    uint32_t destination;
    enum lux_command command;
    uint8_t index;
    uint8_t payload[LUX_PACKET_MAX_SIZE];
    uint16_t payload_length;
    uint32_t crc;
};

enum lux_flags {
    LUX_ACK   = (1 << 0),
    LUX_RETRY = (1 << 1),
};

// Open a serial-port lux channel. Tries /dev/ttyACM* and /dev/ttyUSB*
// Returns fd on succes, -1 on failure, setting errno
int lux_serial_open();

// Open a network (UDP) lux channel. Address should be an IPv4 address.
// Returns fd on succes, -1 on failure, setting errno
int lux_network_open(const char * address_spec, uint16_t port);

// Close a lux channel fd.
void lux_close(int fd);

// Write a lux packet to the channel without expecting a response.
// `flags` is currently unused.
// `packet->crc` is populated with the CRC, but is otherwise unchanged
// Returns 0 on success and -1 on failure, setting errno
int lux_write(int fd, struct lux_packet * packet, enum lux_flags flags);

// Write a lux packet to the channel and wait for a response.
// `flags & LUX_ACK`: Exepect the response to be an ack/nak, and return the error code
// `flags & LUX_RETRY`: Retry sending the message if there was no response or it was invalid
// Returns -1 on failure and 0 on success if LUX_ACK is not set.
// If LUX_ACK is set, the error code from the response (0 <= rc <= 255, 0 is success) is returned.
int lux_command(int fd, struct lux_packet * packet, struct lux_packet * response, enum lux_flags flags);

// Timeout (in milliseconds) to wait for a response from commands
extern int lux_timeout_ms;
