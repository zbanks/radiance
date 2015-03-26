#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <termios.h>
#include "lux_hal.h"

extern int ser;

char serial_init();
int serial_set_attribs (int, int, int);
void serial_set_blocking (int, int);


#endif
