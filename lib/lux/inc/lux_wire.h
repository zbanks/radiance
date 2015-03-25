#ifndef __LUX_WIRE_H__
#define __LUX_WIRE_H__

#include <stdint.h>

#define LUX_PACKET_MAX_SIZE 1024

enum __attribute__((__packed__)) lux_command {
    // General Lux Commands
    CMD_GET_ID = 0x00,
    CMD_RESET,
    CMD_BOOTLOADER,

    // Configuration commands
    CMD_WRITE_CONFIG = 0x10,
    CMD_WRITE_CONFIG_ACK,

    CMD_GET_ADDR,
    CMD_SET_ADDR,
    CMD_SET_ADDR_ACK,

    CMD_GET_USERDATA,
    CMD_SET_USERDATA,
    CMD_SET_USERDATA_ACK,

    CMD_GET_PKTCNT,
    CMD_RESET_PKTCNT,
    CMD_RESET_PKTCNT_ACK,

    // Strip-specific configuration
    CMD_SET_LENGTH = 0x20,
    CMD_GET_LENGTH,
    CMD_SET_LENGTH_ACK,

    // Bootloader-specific commands
    CMD_INVALIDATEAPP = 0x80,
    CMD_FLASH_ERASE,
    CMD_FLASH_WRITE,
    CMD_FLASH_READ,

    // Strip-specific commands
    CMD_FRAME = 0x90,
    CMD_FRAME_ACK,

    CMD_SET_LED,
    CMD_SET_LED_ACK,

    CMD_GET_BUTTON,
};

union lux_command_frame {
	struct __attribute__((__packed__)) cmd_memseg {
        enum lux_command cmd;
		uint32_t addr;
		uint16_t len;
        uint8_t data[];
	} memseg;
	struct __attribute__((__packed__)) cmd_carray {
        enum lux_command cmd;
        uint8_t data[];
	} carray;
	struct __attribute__((__packed__)) cmd_warray {
        enum lux_command cmd;
        uint32_t data[];
	} warray;
	struct __attribute__((__packed__)) cmd_csingle {
        enum lux_command cmd;
        uint8_t data;
	} csingle;
	struct __attribute__((__packed__)) cmd_ssingle {
        enum lux_command cmd;
        uint16_t data;
	} ssingle;
	struct __attribute__((__packed__)) cmd_wsingle {
        enum lux_command cmd;
        uint32_t data;
	} wsingle;
    uint8_t raw[LUX_PACKET_MAX_SIZE];
};


#endif
