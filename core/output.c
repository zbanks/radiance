#include "output.h"
#define _BSD_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h> 
#include <string.h> 


#include "slice.h"
#include "slot.h"
#include "err.h"
#include "crc.h"
#include "lux.h"

int output_running;

color_t** output_buffers = 0;

static pthread_t output_thread;

static pthread_attr_t attr;

static int serial_set_attribs (int, int, int);
static void serial_set_blocking (int, int);

static int ser;
 
static void* output_run(void* args)
{
    while(output_running)
    {
        for(int i=0; i<n_output_strips; i++)
        {
            output_to_buffer(&output_strips[i], output_buffers[i]);
        }
        // TODO: Lux goes here
        write(ser, "\x00\x00\x00", 3);
        usleep(1000);
    }
    return 0;
}

void output_start()
{
    output_buffers = malloc(sizeof(color_t*) * n_output_strips);

    if(!output_buffers) FAIL("Could not allocate output buffer array");

    for(int i=0; i<n_output_strips; i++)
    {
        output_buffers[i] = malloc(sizeof(color_t) * output_strips[i].length);
        if(!output_buffers[i]) FAIL("Could not allocate output buffer");
    }

    output_running = 1;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if(pthread_create(&output_thread, &attr, &output_run, 0)) FAIL("Could not create output thread");

    ser = open("/dev/ttyUSB1", O_RDWR | O_NOCTTY | O_SYNC);
    if(ser < 0) FAIL("error opening port: %d", ser);

    serial_set_attribs(ser, 3000000, 0);
    serial_set_blocking(ser, 1);
}

void output_stop()
{
    output_running = 0;

    pthread_join(output_thread, 0);

    pthread_attr_destroy(&attr);

    for(int i=0; i<n_output_strips; i++)
    {
        free(output_buffers[i]);
    }

    free(output_buffers);
}


int serial_set_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0) FAIL("error %d from tcgetattr", errno);

        //cfsetospeed (&tty, speed);
        //cfsetispeed (&tty, speed);

        tty.c_ispeed = speed;
        tty.c_ospeed = speed;

        tty.c_cflag = (tty.c_cflag & ~(CSIZE | CBAUD)) | CS8 ;     // 8-bit chars
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
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0) FAIL("error %d from tcsetattr", errno);
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

