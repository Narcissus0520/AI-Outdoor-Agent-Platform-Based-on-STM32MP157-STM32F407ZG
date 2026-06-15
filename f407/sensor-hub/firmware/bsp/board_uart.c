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
