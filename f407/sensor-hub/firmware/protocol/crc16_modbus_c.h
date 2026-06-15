#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16_modbus_c(const uint8_t* data, size_t length);

#ifdef __cplusplus
}
#endif
