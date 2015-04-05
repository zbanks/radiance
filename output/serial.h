#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <termios.h>
#include "lux_hal.h"
#include "lux_wire.h"

extern int ser;

char serial_init();
void serial_close();

struct lux_frame {
    int destination;
    int length;
    union lux_command_frame data;
};

char lux_tx_packet(struct lux_frame *);
char lux_rx_packet(struct lux_frame *, int timeout_ms);
char lux_command_ack(struct lux_frame *cmd, int timeout_ms);
char lux_command_response(struct lux_frame *cmd, struct lux_frame *response, int timeout_ms);

#endif
