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

    return HAL_UART_Transmit(&huart4,
                             (uint8_t*)data,
                             (uint16_t)length,
                             BOARD_UART_TIMEOUT_MS) == HAL_OK
               ? 0
               : -1;
}

int board_uart_receive_byte(uint8_t* out_byte)
{
    HAL_StatusTypeDef status;

    if (out_byte == NULL) {
        return -1;
    }

    status = HAL_UART_Receive(&huart4, out_byte, 1U, 0U);
    if (status == HAL_OK) {
        return 1;
    }
    if (status == HAL_TIMEOUT) {
        return 0;
    }
    return -1;
}
