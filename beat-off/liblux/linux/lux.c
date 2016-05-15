#include <errno.h>
#include <fcntl.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#include "linux/crc.h"
#include "linux/lux.h"

//#define DEBUG(x) 
#define STR(x) STR2(x)
#define STR2(x) # x
#define DEBUG(x) printf("[debug] line " STR(__LINE__) ": " x "\n")

static int serial_set_attribs(int fd) {
        struct termios tty;

        memset (&tty, 0, sizeof tty);
        
        tcgetattr(fd, &tty);
        cfsetispeed(&tty, 0010015);
        cfsetospeed(&tty, 0010015);

        //ioctl(3, SNDCTL_TMR_TIMEBASE or SNDRV_TIMER_IOCTL_NEXT_DEVICE or TCGETS, {c_iflags=0, c_oflags=0x4, c_cflags=0x1cbd, c_lflags=0, c_line=0, c_cc[VMIN]=1, c_cc[VTIME]=2, c_cc="\x03\x1c\x7f\x15\x04\x02\x01\x00\x11\x13\x1a\x00\x12\x0f\x17\x16\x00\x00\x00"}) = 0

        tty.c_cflag &= ~CSIZE;     // 8-bit chars
        tty.c_cflag |= CS8;     // 8-bit chars
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~CSTOPB; // 1 stop bit
        tty.c_cflag &= ~(PARENB|PARODD);
        tty.c_iflag &= ~(INPCK|ISTRIP);
        tty.c_iflag &= ~(IXON | IXOFF);
        tty.c_iflag |= IXANY;

        if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;

        return 0;
}

int lux_serial_open() {
    /* 
     * Initialize a new serial port (if an additional one exists)
     * Returns a ser_t instance if successful
     * Returns NULL if there are no available ports or there is an error
     */

    int fd;

    char dbuf[32];
    for(int i = 0; i < 9; i++) {
        sprintf(dbuf, "/dev/ttyUSB%d", i);
        fd = open(dbuf, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
        if(fd >= 0) {
            printf("Found output on '%s', %d\n", dbuf, fd);
            break;
        }

        sprintf(dbuf, "/dev/ttyACM%d", i);
        fd = open(dbuf, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
        if(fd >= 0) {
            printf("Found output on '%s', %d\n", dbuf, fd);
            break;
        }
    }

    if(fd < 0) return -1;

    if(serial_set_attribs(fd) < 0) return -2;

    return fd;
}

void lux_close(int fd) {
    close(fd);
}

int lux_network_open(const char * address, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address);
    addr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
    int rc = connect(sock, (struct sockaddr *) &addr, sizeof addr);
    if (rc < 0) return -1;

    return sock;
}

static int cobs_encode(uint8_t* in_buf, int n, uint8_t* out_buf) {
    int out_ptr = 0;
    uint8_t ctr = 1;
    for(int i = 0; i < n; i++) {
        if(in_buf[i] == 0) {
            out_buf[out_ptr] = ctr;
            out_ptr += ctr;
            ctr = 1;
        } else {
            out_buf[out_ptr + ctr] = in_buf[i];
            ctr++;
            if(ctr == 255) {
                out_buf[out_ptr] = ctr;
                out_ptr += ctr;
                ctr = 1;
            }
        }
    }
    out_buf[out_ptr] = ctr;
    out_ptr += ctr;
    return out_ptr; // success
}

static int cobs_decode(uint8_t* in_buf, int n, uint8_t* out_buf) {
    int out_ptr = 0;
    uint8_t total = 255;
    uint8_t ctr = 255;

    for(int i = 0; i < n; i++) {
        if(in_buf[i] == 0) {
            DEBUG("Invalid character\n");
            errno = EINVAL;
            return -1;
        }

        if(ctr == total) {
            if(total < 255) {
                out_buf[out_ptr++] = 0;
            }
            total = in_buf[i];
            ctr = 1;
        } else {
            out_buf[out_ptr++] = in_buf[i];
            ctr++;
        }
    }

    if(ctr != total) {
        DEBUG("Generic decode error\n");
        errno = EINVAL;
        return -2;
    }

    return out_ptr; // success
}

static int frame(struct lux_packet * packet, uint8_t buffer[static 2048])
{
    uint8_t tmp[2048];
    uint8_t * ptr = tmp;
    int n;

    memcpy(ptr, &packet->destination, sizeof packet->destination);
    ptr += sizeof packet->destination;

    memcpy(ptr, &packet->command, sizeof packet->command);
    ptr += sizeof packet->command;

    memcpy(ptr, &packet->index, sizeof packet->index);
    ptr += sizeof packet->index;

    memcpy(ptr, &packet->payload, packet->payload_length);
    ptr += packet->payload_length;

    crc_t crc = crc_init();
    crc = crc_update(crc, tmp, ptr - tmp);
    crc = crc_finalize(crc);
    packet->crc = crc;
    memcpy(ptr, &crc, sizeof packet->crc);
    ptr += sizeof packet->crc;

    n = cobs_encode(tmp, ptr - tmp, buffer);
    if(n < 0) return n;

    buffer[n] = 0;
    return n + 1; // success
}

static int unframe(uint8_t * raw_data, int raw_len, struct lux_packet * packet) {
    uint8_t tmp[2048];
    int len;

    len = cobs_decode(raw_data, raw_len, tmp);
    if(len < 0) return len;

    if(len < 8) {
        DEBUG("");
        errno = EINVAL;
        return -1;
    }

    crc_t crc = crc_init();
    crc = crc_update(crc, tmp, len);
    crc = crc_finalize(crc);
    if(crc != 0x2144DF1C) {
        DEBUG("Bad CRC");
        errno = EINVAL;
        return -1;
    }

    uint8_t * ptr = tmp;
    memcpy(&packet->destination, ptr, sizeof packet->destination);
    ptr += sizeof packet->destination;

    memcpy(&packet->command, ptr, sizeof packet->command);
    ptr += sizeof packet->command;

    memcpy(&packet->index, ptr, sizeof packet->index);
    ptr += sizeof packet->index;

    packet->payload_length = len - 10;
    memcpy(&packet->payload, ptr, packet->payload_length);
    ptr += packet->payload_length;

    memcpy(&packet->crc, ptr, sizeof packet->crc);

    return packet->payload_length;
}

static int lowlevel_read(int fd, uint8_t data[static 2048]) {
    // XXX assumes only one device is speaking at once! :(
    uint8_t * rx_ptr = data;
    struct timeval tv = {1, 150000}; // 150ms
    fd_set rfds;
    int r;
    int n = 0;
    uint8_t * null;


    do {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        r = select(fd + 1, &rfds, NULL, NULL, &tv);
        if(r < 0) return r;

        if(r == 0) {
            // Read timeout
            DEBUG("Read timeout");
            errno = ETIMEDOUT;
            return n;
        }

        r = read(fd, rx_ptr, 2048 - n);
        if(r < 0 && errno != EAGAIN) return -1;

        n += r;
        rx_ptr += r;

        if (n >= 2048) return n;
    } while((null = memchr(data, 0, n)) == NULL);

    // TODO: Should buffer unused data
    return null - data;
}

static int lowlevel_write(int fd, uint8_t* data, int len) {
    int n_written;
    int total_written = 0;

    while(len > 0) {
        n_written = write(fd, data, len);
        if (n_written < 0 && errno != EAGAIN) return n_written;

        len -= n_written;
        data += n_written;
        total_written += n_written;
    }

    return total_written;
}

static int lux_read(int fd, struct lux_packet * packet) {
    uint8_t rx_buf[2048];
    int r;
    r = lowlevel_read(fd, rx_buf);
    if(r < 0) return r;

    r = unframe(rx_buf, r, packet);
    if(r < 0) return r;

    return r;
}

static int clear_rx(int fd) {
    // Completely clear the RX buffer
    uint8_t rx_buf[2048];
    int n;
    return 0;
    do
    {
        n = read(fd, rx_buf, 2048);
        if (n < 0) return n;
    } while(n);

    return 0; // Successfully flushed
}

int lux_write(int fd, struct lux_packet * packet) {
    uint8_t tx_buf[2048];
    int r;

    r = clear_rx(fd);
    if(r < 0) return r;

    r = frame(packet, tx_buf);
    if(r < 0) return r;

    r = lowlevel_write(fd, tx_buf, r);
    if(r < 0) return r;

    return 0; // Success
}

int lux_command(int fd, struct lux_packet * packet, int retry, struct lux_packet * response) {
    int r;
    if (retry < 0) retry = 1;

    for(int i = 0; i < retry; i++) {
        r = lux_write(fd, packet);
        if(r < 0) continue;

        r = lux_read(fd, response);
        if(r < 0) continue;
        if(response->destination != 0) {
            printf("Invalid destination %#08X\n", response->destination);
            continue;
        }

        return r; // Success
    }

    return -1;
}

int lux_command_ack(int fd, struct lux_packet * packet, int retry) {
    struct lux_packet response;
    int r;

    r = lux_command(fd, packet, retry, &response);
    if (r < 0) return r;

    if (response.payload_length < 5) return -1;
    if (memcmp(&packet->crc, &response.payload[1], sizeof packet->crc) != 0) return -1;

    return response.payload[0];
}
