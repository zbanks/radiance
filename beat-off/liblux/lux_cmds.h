#ifndef __LUX_WIRE_H__
#define __LUX_WIRE_H__

#include <stdint.h>

#define LUX_PACKET_MAX_SIZE 1024

enum __attribute__((__packed__)) lux_command {
    // -------------- General Lux Commands --------------
    
    // CMD_GET_ID: no index, no request payload, varlen data response
    // - Request: Empty
    // - Response: A string identifier of the node type, up to 32 characters
    CMD_GET_ID = 0x00,

    // CMD_GET_DESCRIPTOR: index, no request payload, varlen data response
    // - Request: Empty
    // - Response: Index-based slice of the device descriptor buffer
    CMD_GET_DESCRIPTOR = 0x01,

    // CMD_RESET: no index, 1-byte request payload, ack+crc response
    // - Request: A single byte "flags". Default is 0x00
    // - Response: ack+crc
    // Reboot the node. Flags are node-defined (trigger bootloader, stay off, etc.)
    CMD_RESET = 0x02,

    // -------------- Configuration commands --------------
    // Any changes made to configuration must be committed to nonvolatile storage with CMD_COMMIT_CONFIG
    
    // CMD_COMMIT_CONFIG: no index, no request payload, ack+crc response
    // - Request: Empty
    // - Response: ack+crc
    // Commit config changes to nonvolatile storage.
    CMD_COMMIT_CONFIG = 0x10,

    // CMD_GET_ADDR: no index, no request payload, 72-byte data response
    // - Request: Empty
    // - Response: Address configuration, 72 bytes ( u32[1 + 1 + 16] )
    // { mcast_addr, mcast_mask, unicast[16] }
    CMD_GET_ADDR = 0x11,

    // CMD_SET_ADDR: no index, 72-byte request payload, ack+crc response
    // - Request: Address configuration, 72 bytes ( u32[1 + 1 + 16] )
    // - Response: ack+crc
    CMD_SET_ADDR = 0x12,

    /*
    CMD_GET_USERDATA,
    CMD_SET_USERDATA,
    */

    // CMD_GET_PKTCNT: no index, no request payload, 20-byte data response
    // - Request: Empty
    // - Response: Packet statistics. 20 bytes (u32[5])
    // { good, malformed, overrun, bad_crc, rx_interrupted }
    CMD_GET_PKTCNT = 0x13,

    // CMD_RESET_PKTCNT: no index, no request payload, ack+crc response
    // - Request: Empty
    // - Response: ack+crc
    // Resets all packet counts to 0
    CMD_RESET_PKTCNT = 0x14,

    // -------------- Bootloader-specific commands --------------
    
    // CMD_INVALIDATEAPP: no index, no request payload, ack+crc response
    // - Request: Empty
    // - Response: ack+crc
    // Invalidate loaded app. Future resets will boot into bootloader until flashing is done
    CMD_INVALIDATEAPP = 0x80,

    // CMD_FLASH_BASEADDR: no index, 4-byte request payload, ack+crc response
    // - Request: Base address, 4 bytes , u32
    // - Response: ack+crc
    // Set base address for future FLASH_WRITE commands
    CMD_FLASH_BASEADDR = 0x81,

    // CMD_FLASH_ERASE: no index, 4-byte request payload, ack+crc response
    // - Request: Page address, 4 bytes, u32
    // - Response: ack+crc
    // Erase page at the given address. Pages must be erased before written to
    CMD_FLASH_ERASE = 0x82,

    // CMD_FLASH_WRITE: index, varlen request payload, ack+crc response
    // - Request: Data, to write to ($base_address + 1024 * $index)
    // - Response: ack+crc
    // Write data into flash. Page(s) must be erased before written to
    CMD_FLASH_WRITE = 0x83,

    // CMD_FLASH_READ: no index, 6-byte request payload, varlen response
    // - Request: Address & length to read from (u32 address, u16 length)
    // - Response: $length bytes from address $address
    // Read back data from flash for verification.
    CMD_FLASH_READ = 0x84,

    // -------------- Strip-specific commands --------------

    // CMD_FRAME_FLIP[_ACK]: no index, no request payload, no/ack+crc response
    // - Request: Empty
    // - Response: None for CMD_FRAME_FLIP; ack+crc for CMD_FRAME_FLIP_ACK
    // Flip buffer so frame previously loaded with CMD_FRAME_HOLD is output.
    CMD_FRAME_FLIP = 0x90,      //TODO
    CMD_FRAME_FLIP_ACK = 0x91,  //TODO
    
    // CMD_FRAME[_HOLD][_ACK]: index, varlen request payload, no/ack+crc response
    // - Request: LED strip frame data
    // - Response: None for CMD_FRAME[_HOLD]; ack+crc for CMD_FRAME[_HOLD]_ACK
    // CMD_FRAME: Immediately output; CMD_FRAME_HOLD: store until CMD_FRAME_FLIP command
    CMD_FRAME = 0x92,
    CMD_FRAME_ACK = 0x93,
    CMD_FRAME_HOLD = 0x94,      //TODO
    CMD_FRAME_HOLD_ACK = 0x95,  //TODO

    // CMD_SET_LED: no index, 1-byte request payload, ack+crc response
    // - Request: 0x01 for on, 0x00 for off. All other values reserved.
    // - Response: ack+crc
    // Status LED
    CMD_SET_LED = 0x96,

    // CMD_GET_BUTTON_COUNT: no index, no request payload, 4 byte response
    // - Request: Empty
    // - Response: Number of times the button has been pushed since startup
    CMD_GET_BUTTON_COUNT = 0x97, //TODO (currently "is button pressed?"

    // Configuration
    // TODO: Move to descriptors
    CMD_SET_LENGTH = 0x9C,
    CMD_GET_LENGTH = 0x9D,
};

#endif
