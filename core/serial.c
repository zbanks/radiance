#include "serial.h"
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#include "crc.h"
#include "err.h"
#include "lux.h"

int ser;
static crc_t crc;

void serial_init(){
    ser = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_SYNC);
    if(ser < 0) FAIL("error opening port: %d", ser);

    serial_set_attribs(ser, 3000000, 1);
    serial_set_blocking(ser, 1);
}

void lux_hal_enable_rx(){
    int status;
    ioctl(ser, TIOCMGET, &status);
    status &= ~TIOCM_RTS;
    ioctl(ser, TIOCMSET, status);
};

void lux_hal_disable_rx(){
    int status;
    ioctl(ser, TIOCMGET, &status);
    status |= TIOCM_RTS;
    ioctl(ser, TIOCMSET, status);
};

void lux_hal_enable_tx(){};
void lux_hal_disable_tx(){};

int16_t lux_hal_bytes_to_read(){
    int bytes_avail;
    ioctl(ser, FIONREAD, &bytes_avail);
    return bytes_avail;
}

uint8_t lux_hal_read_byte(){
    uint8_t byte = 0;
    read(ser, &byte, 1);
    return byte;
}

int16_t lux_hal_bytes_to_write(){
    return 2048;
}

void lux_hal_write_byte(uint8_t byte){
    write(ser, &byte, 1);
}

uint8_t lux_hal_tx_flush(){
    tcflush(ser, TCOFLUSH);
    return 1;
}

void lux_hal_reset_crc(){
    crc = crc_init();
}

void lux_hal_crc(uint8_t byte){
    crc = crc_update(crc, &byte, 1);
}

uint8_t lux_hal_crc_ok(){
    return crc_finalize(crc) == 0x2144DF1C; 
}

void lux_hal_write_crc(uint8_t* ptr){
    memcpy(ptr, &crc, 4);
}

int serial_set_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        struct serial_struct serinfo;
        int closestSpeed;
        memset (&tty, 0, sizeof tty);
        /*
        if (tcgetattr (fd, &tty) != 0) FAIL("error %d from tcgetattr", errno);

        
        tty.c_cflag = (tty.c_cflag & ~CSIZE ) | CS8 ;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        //tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0) FAIL("error %d from tcsetattr", errno);
        */

        

        ioctl(ser, TIOCGSERIAL, &serinfo);
        serinfo.flags = (serinfo.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
        serinfo.custom_divisor = (serinfo.baud_base + (speed / 2)) / speed;
        closestSpeed = serinfo.baud_base / serinfo.custom_divisor;
        printf("speed: %d\n", closestSpeed);
        
        fcntl(fd, F_SETFL, 0);
        tcgetattr(fd, &options);
        cfsetispeed(&options, speed ?: B38400);
        cfsetospeed(&options, speed ?: B38400);
        cfmakeraw(&options);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~CRTSCTS;
        if (tcsetattr(fd, TCSANOW, &options) != 0)
            return -1;

        ioctl(ser, TIOCSSERIAL, &serinfo);

        cfsetospeed (&tty, B38400);
        cfsetispeed (&tty, B38400);

        lux_hal_disable_rx();

        return 0;
}

void serial_set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0) FAIL("error %d from tggetattr", errno);

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 2;            // 0.2 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0) FAIL("error %d setting term attributes", errno);
}

