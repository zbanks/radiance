#ifndef __LUX_HAL_H
#define __LUX_HAL_H

#include <stdint.h>

// Number of bytes in the CRC
#define LUX_HAL_CRC_SIZE 4

// Extra space for CRC
#define LUX_PACKET_MEMORY_ALLOCATED_SIZE (LUX_PACKET_MEMORY_SIZE+LUX_HAL_CRC_SIZE)

// Enable / disable the receive / transmit hardware
void lux_hal_enable_rx();
void lux_hal_disable_rx();
void lux_hal_enable_tx();
void lux_hal_disable_tx();

// Return number of bytes that can be read
int16_t lux_hal_bytes_to_read();

// Read a byte
uint8_t lux_hal_read_byte();

// Return the number of bytes that can be written
int16_t lux_hal_bytes_to_write();

// Write a byte
void lux_hal_write_byte(uint8_t byte);

// Flush remaining bytes (call repeatedly; returns 1 on success)
uint8_t lux_hal_tx_flush();

// Resets CRC calculation
void lux_hal_reset_crc();

// Update CRC calculation with a new datum
void lux_hal_crc(uint8_t byte);

// Returns 1 if CRC is good
uint8_t lux_hal_crc_ok();

// Writes the current CRC to memory at ptr[0..3]
void lux_hal_write_crc(uint8_t* ptr);

#endif
