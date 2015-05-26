#include "lux.h"

#include "lux_hal.h"

// Global variables

// Useful buffers for the application
uint8_t lux_destination[LUX_DESTINATION_SIZE];
uint8_t lux_packet[LUX_PACKET_MEMORY_ALLOCATED_SIZE];
uint16_t lux_packet_length;

// Counters for problematic data
uint32_t lux_good_packet_counter;
uint32_t lux_malformed_packet_counter;
uint32_t lux_packet_overrun_counter;
uint32_t lux_bad_checksum_counter;
uint32_t lux_rx_interrupted_counter;

// Flag indicating whether there is currently a received packet in lux_packet
uint8_t lux_packet_in_memory;

// Callback function pointers
uint8_t (*lux_fn_match_destination)(uint8_t* dest);
void (*lux_fn_rx)(); 

// Local variables

// The current state of the encoder / decoder (codec)
static enum
{
    CODEC_START,
    READ_DESTINATION,
    READ_PAYLOAD,
    SKIP_CURRENT_PACKET,
    ENCODE,
    ENCODER_STALLED,
} codec_state;

// The current sub-state of the encoder (within ENCODE / ENCODER_STALLED)
static enum
{
    ENCODER_START,
    WRITE_DESTINATION,
    WRITE_PAYLOAD,
    WRITE_CRC,
    FLUSH,
} encoder_state;

// Pointer to current index in lux_packet
static int16_t packet_pointer;
// Pointer to current index in lux_destination
static int8_t destination_pointer;
// Temporary home for destination until we can be sure it matches
static uint8_t tmp_destination[LUX_DESTINATION_SIZE];

// Number of bytes remaining in the COBS block
static uint8_t cobs_remaining;
// Flag indicating whether or not a zero byte is due at the end of this COBS block
static uint8_t cobs_add_zero;

// Buffer and pointers for the COBS encoder to hold the current block
static uint8_t cobs_encoder_fill_ptr;
static uint8_t cobs_encoder_send_ptr;
static uint8_t cobs_buffer[256];

// Local functions

// Prepare COBS decoder for the start of a new block
static void reset_cobs_decoder()
{
    cobs_remaining=0;
    cobs_add_zero=0;
}

// Decode one COBS byte from src, and put the result in *dest
// Returns 1 if *dest was populated, 0 otherwise.
static uint8_t cobs_decode(uint8_t src, uint8_t* dest)
{
    uint8_t result;
    if(!cobs_remaining)
    {
        cobs_remaining=src-1;
        result=cobs_add_zero;
        if(cobs_add_zero)
        {
            lux_hal_crc(0);
            *dest=0;
        }
        cobs_add_zero = (src < 255);
        return result;
    }

    cobs_remaining--;
    lux_hal_crc(src);
    *dest=src;
    return 1;
}

// Prepare the COBS encoder for the start of a new block
static void reset_cobs_encoder()
{
    cobs_encoder_fill_ptr=1;
}

// Write out as much of the remaining bytes in the COBS encoder buffer as possible.
// Returns 1 if the message was completely transmitted.
static uint8_t write_continuation()
{
    while(cobs_encoder_send_ptr<cobs_encoder_fill_ptr && lux_hal_bytes_to_write())
    {
        lux_hal_write_byte(cobs_buffer[cobs_encoder_send_ptr++]);
    }

    if(cobs_encoder_send_ptr == cobs_encoder_fill_ptr)
    {
        reset_cobs_encoder();
        return 1;
    }
    return 0;
}

// Write out the current COBS encoder buffer.
// Returns 1 if the message was completely transmitted.
// If the message was not completely transmitted, subsequent calls to
// write_continuation are necessary before additional COBS encoding
// can be performed.
static uint8_t write()
{
    cobs_encoder_send_ptr=0;
    return write_continuation();
}

// Encode a byte using COBS and send it (buffered by block.)
// Returns 1 if additional bytes can be written.
// If this function returns 0, subsequent calls to
// write_continuation are necessary before additional COBS encoding
// can be performed.
static uint8_t cobs_encode_and_send(uint8_t byte)
{
    lux_hal_crc(byte);

    if(byte == 0)
    {
        goto write;
    }
    else
    {
        //TODO: Needs testing
        //XXX cobs_encoder_fill_ptr probably needs to get reset?
        cobs_buffer[cobs_encoder_fill_ptr]=byte;
        if(cobs_encoder_fill_ptr == 255){
            goto write;
        }
        cobs_encoder_fill_ptr++;
    }
    return 1;

    write:
    cobs_buffer[0]=cobs_encoder_fill_ptr;
    return write();
}

// Finishes the current COBS block, adds a null terminator, and writes it out.
// Returns 1 if the message was completely transmitted.
// If the message was not completely transmitted, subsequent calls to
// write_continuation are necessary before additional COBS encoding
// can be performed.
static uint8_t cobs_encode_flush()
{
    cobs_buffer[0]=cobs_encoder_fill_ptr;
    cobs_buffer[cobs_encoder_fill_ptr++]=0;
    return write();
}

// Global functions

void lux_init()
{
    lux_reset_counters();
    lux_hal_enable_rx();

    codec_state=CODEC_START;
}

// Do any necessary work to handle incoming or outgoing data
// Calls *lux_fn_rx() when a packet has arrived.
// Call this repeatedly in your application's main loop.
void lux_codec()
{
    uint8_t byte_read;
    uint8_t decoded_byte;

    switch(codec_state)
    {
        case CODEC_START:
        read_destination:
            codec_state=READ_DESTINATION;
            destination_pointer=0;
            lux_hal_reset_crc();
            reset_cobs_decoder();
        case READ_DESTINATION:
            while(lux_hal_bytes_to_read())
            {
                byte_read=lux_hal_read_byte();
                if(!byte_read) goto read_destination; // Don't bother incrementing any counters because this is probably noise
                if(cobs_decode(byte_read,&decoded_byte))
                {
                    tmp_destination[destination_pointer++]=decoded_byte;
                }
                if(destination_pointer == LUX_DESTINATION_SIZE)
                {
                    if((*lux_fn_match_destination)(tmp_destination))
                    {
                        if(lux_packet_in_memory)
                        {
                            lux_packet_overrun_counter++;
                            goto skip_current_packet;
                        }
                        else
                        {
                            for(destination_pointer=0;destination_pointer<LUX_DESTINATION_SIZE;destination_pointer++)
                            {
                                lux_destination[destination_pointer]=tmp_destination[destination_pointer];
                            }
                            goto read_payload;
                        }
                    }
                    goto skip_current_packet;
                }
            }
            break;

        read_payload:
        codec_state=READ_PAYLOAD;
        packet_pointer=0;
        case READ_PAYLOAD:
            while(lux_hal_bytes_to_read())
            {
                byte_read=lux_hal_read_byte();
                if(!byte_read)
                {
                    if(lux_hal_crc_ok())
                    {
                        lux_packet_length=packet_pointer-LUX_HAL_CRC_SIZE;
                        lux_packet_in_memory=1;
                        lux_good_packet_counter++;
                        (*************lux_fn_rx)();
                        if(codec_state == ENCODE) goto encode;
                        goto read_destination;
                    }
                    lux_bad_checksum_counter++;
                    goto read_destination;
                }
                if(cobs_decode(byte_read,&decoded_byte))
                {
                    if(packet_pointer < LUX_PACKET_MEMORY_ALLOCATED_SIZE)
                    {
                        lux_packet[packet_pointer++]=decoded_byte;
                    }
                    else
                    {
                        lux_malformed_packet_counter++;
                        goto skip_current_packet;
                    }
                }
            }
            break;
        skip_current_packet:
        codec_state=SKIP_CURRENT_PACKET;
        case SKIP_CURRENT_PACKET:
            while(lux_hal_bytes_to_read())
            {
                if(!lux_hal_read_byte()) goto read_destination;
            }
            break;

        encode:
        codec_state=ENCODE;
        case ENCODE:
            switch(encoder_state)
            {
                case ENCODER_START:
                    destination_pointer=0;
                    reset_cobs_encoder();
                    lux_hal_reset_crc();
                    encoder_state=WRITE_DESTINATION;
                case WRITE_DESTINATION:
                    while(destination_pointer<LUX_DESTINATION_SIZE)
                    {
                        if(!cobs_encode_and_send(lux_destination[destination_pointer++])) break;
                    }
                    packet_pointer=0;
                    encoder_state=WRITE_PAYLOAD;
                case WRITE_PAYLOAD:
                    while(packet_pointer<lux_packet_length)
                    {
                        if(!cobs_encode_and_send(lux_packet[packet_pointer++])) break;
                    }
                    lux_hal_write_crc(&lux_packet[packet_pointer]);
                    encoder_state=WRITE_CRC;
                case WRITE_CRC:
                    while(packet_pointer<lux_packet_length+LUX_HAL_CRC_SIZE)
                    {
                        if(!cobs_encode_and_send(lux_packet[packet_pointer++])) break;
                    }
                    encoder_state=FLUSH;
                    if(!cobs_encode_flush()) break; // flush COBS
                case FLUSH:
                    if(lux_hal_tx_flush())
                    {
                        lux_hal_disable_tx();
                        lux_hal_enable_rx();
                        goto read_destination;
                    }
            }
            break;

        case ENCODER_STALLED:
            if(write_continuation()) goto encode;
            break;
    }
}

// Call this function before altering lux_packet or lux_destination
void lux_stop_rx()
{
    if(lux_hal_bytes_to_read())
    {
        lux_rx_interrupted_counter++;
    }
    lux_hal_disable_rx();
}

// Output the packet stored in lux_packet to lux_destination and re-enable the receiver
void lux_start_tx()
{
    // Busywait: wait for RTS to go low
    for(volatile long i = 0; i < 100000; i++); //FIXME
    codec_state=ENCODE;
    encoder_state=ENCODER_START;
    lux_hal_enable_tx();
}

// Reset all of the good/bad packet counters to zero
void lux_reset_counters(){
    lux_good_packet_counter = 0;
    lux_malformed_packet_counter = 0;
    lux_packet_overrun_counter = 0;
    lux_bad_checksum_counter = 0;
    lux_rx_interrupted_counter = 0;
}
