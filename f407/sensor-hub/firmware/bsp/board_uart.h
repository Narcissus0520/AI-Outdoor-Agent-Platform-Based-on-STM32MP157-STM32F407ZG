#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_uart_init(void);
int board_uart_send_bytes(const uint8_t* data, size_t length);
int board_uart_send_diagnostic_bytes(const uint8_t* data, size_t length);
int board_uart_receive_byte(uint8_t* out_byte);
void board_uart_poll(void);
uint32_t board_uart_primary_tx_drop_count(void);
uint32_t board_uart_diagnostic_tx_drop_count(void);
uint32_t board_uart_rx_drop_count(void);
size_t board_uart_primary_tx_pending_bytes(void);
size_t board_uart_diagnostic_tx_pending_bytes(void);

#ifdef __cplusplus
}
#endif
