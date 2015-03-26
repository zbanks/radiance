#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <termios.h>
#include "lux_hal.h"
#include "lux_wire.h"

extern int ser;

char serial_init();
int serial_set_attribs (int, int);
void serial_set_blocking (int, int);

struct lux_frame {
    int destination;
    int length;
    union lux_command_frame data;
};

char lux_tx_packet(struct lux_frame *);
char lux_rx_packet(struct lux_frame *);
char lux_command_ack(struct lux_frame *cmd);
char lux_command_response(struct lux_frame *cmd, struct lux_frame *response);

#endif
