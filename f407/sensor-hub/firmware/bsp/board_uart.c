#include "bsp/board_uart.h"

#if defined(__GNUC__)
#define SENSOR_HUB_WEAK __attribute__((weak))
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define SENSOR_HUB_WEAK __weak
#else
#define SENSOR_HUB_WEAK
#endif

SENSOR_HUB_WEAK int board_uart_send_bytes(const uint8_t* data, size_t length)
{
    (void)data;
    (void)length;
    return 0;
}

SENSOR_HUB_WEAK int board_uart_send_diagnostic_bytes(const uint8_t* data, size_t length)
{
    (void)data;
    (void)length;
    return 0;
}

SENSOR_HUB_WEAK int board_uart_init(void)
{
    return 0;
}

SENSOR_HUB_WEAK int board_uart_receive_byte(uint8_t* out_byte)
{
    (void)out_byte;
    return 0;
}

SENSOR_HUB_WEAK void board_uart_poll(void)
{
}

SENSOR_HUB_WEAK uint32_t board_uart_primary_tx_drop_count(void)
{
    return 0U;
}

SENSOR_HUB_WEAK uint32_t board_uart_diagnostic_tx_drop_count(void)
{
    return 0U;
}

SENSOR_HUB_WEAK uint32_t board_uart_rx_drop_count(void)
{
    return 0U;
}

SENSOR_HUB_WEAK size_t board_uart_primary_tx_pending_bytes(void)
{
    return 0U;
}

SENSOR_HUB_WEAK size_t board_uart_diagnostic_tx_pending_bytes(void)
{
    return 0U;
}
