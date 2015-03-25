#ifndef __LUX_H
#define __LUX_H

#include <stdint.h>
#include "lux_wire.h"

// How many bytes the destination is
#define LUX_DESTINATION_SIZE 4

// How many bytes the longest packet can be
#define LUX_PACKET_MEMORY_SIZE LUX_PACKET_MAX_SIZE

// Run lux_init once at the start of the program
void lux_init();

// Run lux_codec frequently (polling)
void lux_codec();

// Run lux_stop_rx before filling the packet memory with data to transmit
void lux_stop_rx();

// Run lux_start_tx to send a packet
void lux_start_tx();

// Run lux_reset_counters to reset packet counters to zero
void lux_reset_counters();

// The packet's destination
extern uint8_t lux_destination[LUX_DESTINATION_SIZE];

// The packet's payload
extern uint8_t lux_packet[];

// The packet's length
extern uint16_t lux_packet_length;

// Flag indicating whether a valid packet has been received
// (must be cleared by application before another packet can be received)
extern uint8_t lux_packet_in_memory;

// Counters to keep track of bad things that could happen
extern uint32_t lux_good_packet_counter;
extern uint32_t lux_malformed_packet_counter;
extern uint32_t lux_packet_overrun_counter;
extern uint32_t lux_bad_checksum_counter;
extern uint32_t lux_rx_interrupted_counter;

// Functions that need to be provided:

// This function is called to see if a packet destination matches this device.
extern uint8_t (*lux_fn_match_destination)(uint8_t* dest);

// This function is called when a packet has been received.
// It doesn't have to do anything, but if lux_packet_in_memory
// isn't cleared in a timely fashion, you may drop packets
extern void (*lux_fn_rx)(); 


#endif
