#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_uart_send_bytes(const uint8_t* data, size_t length);
int board_uart_receive_byte(uint8_t* out_byte);

#ifdef __cplusplus
}
#endif
