#include "protocol/mcu_command_decoder_c.h"

#include "protocol/crc16_modbus_c.h"
#include "protocol/mcu_frame_builder_c.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t read_u16_le(const uint8_t* bytes)
{
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static uint32_t read_u32_le(const uint8_t* bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) |
           ((uint32_t)bytes[3] << 24U);
}

static void decoder_reset(mcu_command_decoder_c_t* decoder)
{
    if (decoder != 0) {
        decoder->length = 0U;
    }
}

static void drop_first_byte(mcu_command_decoder_c_t* decoder)
{
    size_t index;
    if (decoder == 0 || decoder->length == 0U) {
        return;
    }
    for (index = 1U; index < decoder->length; ++index) {
        decoder->buffer[index - 1U] = decoder->buffer[index];
    }
    --decoder->length;
}

void mcu_command_decoder_init(mcu_command_decoder_c_t* decoder)
{
    decoder_reset(decoder);
}

int mcu_command_decoder_feed_byte(mcu_command_decoder_c_t* decoder,
                                  uint8_t byte,
                                  mcu_command_c_t* out_command)
{
    uint16_t payload_size;
    size_t frame_size;
    uint16_t expected_crc;
    uint16_t actual_crc;

    if (decoder == 0 || out_command == 0) {
        return -1;
    }

    if (decoder->length >= sizeof(decoder->buffer)) {
        decoder_reset(decoder);
    }

    decoder->buffer[decoder->length++] = byte;

    while (decoder->length >= 2U &&
           (decoder->buffer[0] != MCU_FRAME_SOF0 || decoder->buffer[1] != MCU_FRAME_SOF1)) {
        drop_first_byte(decoder);
    }

    if (decoder->length < MCU_FRAME_HEADER_SIZE) {
        return 0;
    }

    if (decoder->buffer[2] != MCU_PROTOCOL_VERSION) {
        decoder_reset(decoder);
        return -1;
    }

    payload_size = read_u16_le(&decoder->buffer[6]);
    if (payload_size > MCU_FRAME_MAX_PAYLOAD_SIZE) {
        decoder_reset(decoder);
        return -1;
    }

    frame_size = MCU_FRAME_HEADER_SIZE + payload_size + MCU_FRAME_CRC_SIZE;
    if (decoder->length < frame_size) {
        return 0;
    }

    expected_crc = read_u16_le(&decoder->buffer[MCU_FRAME_HEADER_SIZE + payload_size]);
    actual_crc = crc16_modbus_c(&decoder->buffer[2], MCU_FRAME_HEADER_SIZE - 2U + payload_size);
    if (expected_crc != actual_crc) {
        decoder_reset(decoder);
        return -1;
    }

    if (decoder->buffer[3] != MCU_MSG_TYPE_COMMAND_PING ||
        payload_size != MCU_COMMAND_PING_PAYLOAD_SIZE) {
        decoder_reset(decoder);
        return -1;
    }

    out_command->type = decoder->buffer[3];
    out_command->sequence = read_u16_le(&decoder->buffer[4]);
    out_command->nonce = read_u32_le(&decoder->buffer[MCU_FRAME_HEADER_SIZE]);
    decoder_reset(decoder);
    return 1;
}
