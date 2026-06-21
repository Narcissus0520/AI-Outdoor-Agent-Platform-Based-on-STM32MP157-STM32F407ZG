#pragma once

#include "protocol/mcu_frame_builder_c.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t type;
    uint16_t sequence;
    uint32_t nonce;
} mcu_command_c_t;

typedef struct {
    uint8_t buffer[MCU_FRAME_MAX_SIZE];
    size_t length;
} mcu_command_decoder_c_t;

void mcu_command_decoder_init(mcu_command_decoder_c_t* decoder);
int mcu_command_decoder_feed_byte(mcu_command_decoder_c_t* decoder,
                                  uint8_t byte,
                                  mcu_command_c_t* out_command);

#ifdef __cplusplus
}
#endif
