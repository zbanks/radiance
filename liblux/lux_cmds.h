#pragma once

#include <stdint.h>

#define LUX_PACKET_MAX_SIZE 1024

enum __attribute__((__packed__)) lux_command {
    // -------------- General Lux Commands --------------
    
    // LUX_CMD_GET_ID: no index, no request payload, varlen data response
    // - Request: Empty
    // - Response: A string identifier of the node type, up to 32 characters
    LUX_CMD_GET_ID = 0x00,

    // LUX_CMD_GET_DESCRIPTOR: index, no request payload, varlen data response
    // - Request: Empty
    // - Response: Index-based slice of the device descriptor buffer
    LUX_CMD_GET_DESCRIPTOR = 0x01,

    // LUX_CMD_RESET: no index, 1-byte request payload, ack+crc response
    // - Request: A single byte "flags". Default is 0x00
    // - Response: ack+crc
    // Reboot the node. Flags are node-defined (trigger bootloader, stay off, etc.)
    LUX_CMD_RESET = 0x02,

    // -------------- Configuration commands --------------
    // Any changes made to configuration must be committed to nonvolatile storage with LUX_CMD_COMMIT_CONFIG
    
    // LUX_CMD_COMMIT_CONFIG: no index, no request payload, ack+crc response
    // - Request: Empty
    // - Response: ack+crc
    // Commit config changes to nonvolatile storage.
    LUX_CMD_COMMIT_CONFIG = 0x10,

    // LUX_CMD_GET_ADDR: no index, no request payload, 72-byte data response
    // - Request: Empty
    // - Response: Address configuration, 72 bytes ( u32[1 + 1 + 16] )
    // { mcast_addr, mcast_mask, unicast[16] }
    LUX_CMD_GET_ADDR = 0x11,

    // LUX_CMD_SET_ADDR: no index, 72-byte request payload, ack+crc response
    // - Request: Address configuration, 72 bytes ( u32[1 + 1 + 16] )
    // - Response: ack+crc
    LUX_CMD_SET_ADDR = 0x12,

    /*
    LUX_CMD_GET_USERDATA,
    LUX_CMD_SET_USERDATA,
    */

    // LUX_CMD_GET_PKTCNT: no index, no request payload, 20-byte data response
    // - Request: Empty
    // - Response: Packet statistics. 20 bytes (u32[5])
    // { good, malformed, overrun, bad_crc, rx_interrupted }
    LUX_CMD_GET_PKTCNT = 0x13,

    // LUX_CMD_RESET_PKTCNT: no index, no request payload, ack+crc response
    // - Request: Empty
    // - Response: ack+crc
    // Resets all packet counts to 0
    LUX_CMD_RESET_PKTCNT = 0x14,

    // -------------- Bootloader-specific commands --------------
    
    // LUX_CMD_INVALIDATEAPP: no index, no request payload, ack+crc response
    // - Request: Empty
    // - Response: ack+crc
    // Invalidate loaded app. Future resets will boot into bootloader until flashing is done
    LUX_CMD_INVALIDATEAPP = 0x80,

    // LUX_CMD_FLASH_BASEADDR: no index, 4-byte request payload, ack+crc response
    // - Request: Base address, 4 bytes , u32
    // - Response: ack+crc
    // Set base address for future FLASH_WRITE commands
    LUX_CMD_FLASH_BASEADDR = 0x81,

    // LUX_CMD_FLASH_ERASE: no index, 4-byte request payload, ack+crc response
    // - Request: Page address, 4 bytes, u32
    // - Response: ack+crc
    // Erase page at the given address. Pages must be erased before written to
    LUX_CMD_FLASH_ERASE = 0x82,

    // LUX_CMD_FLASH_WRITE: index, varlen request payload, ack+crc response
    // - Request: Data, to write to ($base_address + 1024 * $index)
    // - Response: ack+crc
    // Write data into flash. Page(s) must be erased before written to
    LUX_CMD_FLASH_WRITE = 0x83,

    // LUX_CMD_FLASH_READ: no index, 6-byte request payload, varlen response
    // - Request: Address & length to read from (u32 address, u16 length)
    // - Response: $length bytes from address $address
    // Read back data from flash for verification.
    LUX_CMD_FLASH_READ = 0x84,

    // -------------- Strip-specific commands --------------

    // LUX_CMD_FRAME_SYNC[_ACK]: no index, no request payload, no/ack+crc response
    // - Request: Empty
    // - Response: None for LUX_CMD_FRAME_SYNC; ack+crc for CMD_FRAME_SYNC_ACK
    // Flip buffer so frame previously loaded with LUX_CMD_FRAME_HOLD is output.
    LUX_CMD_SYNC = 0x90,      //TODO
    LUX_CMD_SYNC_ACK = 0x91,  //TODO
    
    // LUX_CMD_FRAME[_HOLD][_ACK]: index, varlen request payload, no/ack+crc response
    // - Request: LED strip frame data
    // - Response: None for LUX_CMD_FRAME[_HOLD]; ack+crc for CMD_FRAME[_HOLD]_ACK
    // LUX_CMD_FRAME: Immediately output; CMD_FRAME_HOLD: store until CMD_FRAME_FLIP command
    LUX_CMD_FRAME = 0x92,
    LUX_CMD_FRAME_ACK = 0x93,
    LUX_CMD_FRAME_HOLD = 0x94,      //TODO
    LUX_CMD_FRAME_HOLD_ACK = 0x95,  //TODO

    // LUX_CMD_SET_LED: no index, 1-byte request payload, ack+crc response
    // - Request: 0x01 for on, 0x00 for off. All other values reserved.
    // - Response: ack+crc
    // Status LED
    LUX_CMD_SET_LED = 0x96,

    // LUX_CMD_GET_BUTTON_COUNT: no index, no request payload, 4 byte response
    // - Request: Empty
    // - Response: Number of times the button has been pushed since startup
    LUX_CMD_GET_BUTTON_COUNT = 0x97, //TODO (currently "is button pressed?"

    // Configuration
    // TODO: Move to descriptors
    LUX_CMD_SET_LENGTH = 0x9C,
    LUX_CMD_GET_LENGTH = 0x9D,
};

// Response format from LUX_CMD_GET_PKTCNT
struct __attribute__((__packed__)) lux_stats_payload {
    uint32_t good_packet;
    uint32_t malformed_packet;
    uint32_t packet_overrun;
    uint32_t bad_checksum;
    uint32_t rx_interrupted;
    uint32_t bad_address;
};

