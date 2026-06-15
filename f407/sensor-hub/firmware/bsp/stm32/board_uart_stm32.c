#include "bsp/board_uart.h"

#include "usart.h"

#include <stdint.h>

enum {
    BOARD_UART_TIMEOUT_MS = 50U,
};

int board_uart_send_bytes(const uint8_t* data, size_t length)
{
    if (data == NULL || length == 0U || length > UINT16_MAX) {
        return -1;
    }

    return HAL_UART_Transmit(&huart1,
                             (uint8_t*)data,
                             (uint16_t)length,
                             BOARD_UART_TIMEOUT_MS) == HAL_OK
               ? 0
               : -1;
}
