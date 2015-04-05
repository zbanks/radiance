#include "serial.h"
#include <SDL/SDL_timer.h>
#include <errno.h>
#include <fcntl.h> 
#include <linux/serial.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "crc.h"
#include "err.h"
#include "lux.h"
#include "lux.h"

int ser;
static char lux_is_transmitting;
static crc_t crc;
int serial_set_attribs (int, int);
void serial_set_blocking (int, int);

uint8_t match_destination(uint8_t* dest){
    uint32_t addr = *(uint32_t *)dest;
    return addr == 0;
}

void rx_packet() {
}

char serial_init(){
    /* 
     * Initialize the serial port (if it exists)
     * Returns 0 if successful
     * Returns nonzero if there are no available ports or there is an error
     */
    char dbuf[32];
    for(int i = 0; i < 9; i++){
        sprintf(dbuf, "/dev/ttyUSB%d", i);
        ser = open(dbuf, O_RDWR | O_NOCTTY | O_SYNC);
        if(ser > 0){
            printf("Found output on '%s', %d\n", dbuf, ser);
            break;
        }
        //if(ser < 0) FAIL("error opening port: %d", ser);
    }

    lux_fn_match_destination = &match_destination;
    lux_fn_rx = &rx_packet;
    lux_init();

    if(ser > 0){
        serial_set_attribs(ser, 3000000);
        serial_set_blocking(ser, 1);
        return 1;
    }
    return 0;
}

void serial_close(){
    //TODO
    //close(ser);
}

char lux_command_response(struct lux_frame *cmd, struct lux_frame *response, int timeout_ms){
    /* 
     * Transmits command `*cmd`, and waits for response and saves it to `*response`
     * Returns 0 on success 
     */
    char r;
    
    if((r = lux_tx_packet(cmd)))
        return r;

    if(response){
        if((r = lux_rx_packet(response, timeout_ms)))
            return r;

        if(response->destination != 0)
            return -10;
    }
    return 0;
}

char lux_command_ack(struct lux_frame *cmd, int timeout_ms){
    /*
     * Transmits command `*cmd`, and waits for ack response
     * Returns 0 on success
     */
    char r;
    struct lux_frame response;

    if((r = lux_command_response(cmd, &response, timeout_ms)))
        return r;

    if(response.destination != 0)
        return -20;

    if(response.length != 1)
        return -21;

    return response.data.raw[0];
}

char lux_tx_packet(struct lux_frame *cmd){
    /*
     * Transmits command `*cmd`
     * Returns 0 on success
     */
    lux_hal_disable_rx();

    if(!cmd)
        return -30;

    *(uint32_t*)lux_destination = cmd->destination;
    if(cmd->length > LUX_PACKET_MAX_SIZE)
        return -31;
    lux_packet_length = cmd->length;
    memcpy(lux_packet, &cmd->data, cmd->length);

    lux_packet_in_memory = 0;
    lux_start_tx();
    while(lux_is_transmitting) lux_codec();
    return 0;
}

char lux_rx_packet(struct lux_frame *response, int timeout_ms){
    /* 
     * Attempts to recieve a packet (with a timeout)
     * Puts recieved packet into `*response`
     * Returns 0 on success
     */

    for(int j = 0; j <= timeout_ms; j++){
        for(int i = 0; i < 1100; i++)
            lux_codec();   
        SDL_Delay(1);
        if(lux_packet_in_memory){
            //printf("rx packet @t=%d\n", j);
            break;
        }
    }

    if(!lux_packet_in_memory)
        return -40;

    lux_packet_in_memory = 0;

    if(!response)
        return -41;

    response->length = lux_packet_length;
    response->destination = *(uint32_t *) lux_destination;
    memset(&response->data, 0, LUX_PACKET_MAX_SIZE);
    memcpy(&response->data, lux_packet, lux_packet_length);

    return 0;
}

void lux_hal_enable_rx(){
    const int r = TIOCM_RTS;
    lux_is_transmitting = 0;
    SDL_Delay(1);
    ioctl(ser, TIOCMBIS, &r);
};

void lux_hal_disable_rx(){
    const int r = TIOCM_RTS;
    lux_is_transmitting = 1;
    ioctl(ser, TIOCMBIC, &r);
    SDL_Delay(1);
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
    crc = crc_finalize(crc);
    memcpy(ptr, &crc, 4);
}

int serial_set_attribs (int fd, int speed)
{
        struct termios tty;

        memset (&tty, 0, sizeof tty);
        
        tcgetattr(fd, &tty);
        cfsetispeed(&tty, speed ?: 0010015);
        cfsetospeed(&tty, speed ?: 0010015);

        tty.c_cflag &= ~CSIZE;     // 8-bit chars
        tty.c_cflag |= CS8;     // 8-bit chars
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~CSTOPB; // 1 stop bit
        tty.c_cflag &= ~(PARENB|PARODD);
        tty.c_iflag &= ~(INPCK|ISTRIP);

        if (tcsetattr(fd, TCSANOW, &tty) != 0)
            return -1;

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

