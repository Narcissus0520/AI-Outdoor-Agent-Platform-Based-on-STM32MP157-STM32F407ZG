#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_uart_send_bytes(const uint8_t* data, size_t length);

#ifdef __cplusplus
}
#endif
