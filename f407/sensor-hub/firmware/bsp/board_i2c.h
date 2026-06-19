#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_i2c_mem_read(uint8_t device_address_7bit, uint8_t register_address, uint8_t* data, size_t length);
int board_i2c_mem_write(uint8_t device_address_7bit, uint8_t register_address, const uint8_t* data, size_t length);

#ifdef __cplusplus
}
#endif
