#include "bsp/board_icm42688_gpio.h"

#if defined(__GNUC__)
#define SENSOR_HUB_WEAK __attribute__((weak))
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define SENSOR_HUB_WEAK __weak
#else
#define SENSOR_HUB_WEAK
#endif

SENSOR_HUB_WEAK int board_icm42688_data_ready(void)
{
    return 0;
}

SENSOR_HUB_WEAK int board_icm42688_interrupt_init(void)
{
    return 0;
}

SENSOR_HUB_WEAK int board_icm42688_take_data_ready_event(void)
{
    return 0;
}

SENSOR_HUB_WEAK void board_icm42688_discard_data_ready_events(void)
{
}

SENSOR_HUB_WEAK uint32_t board_icm42688_data_ready_event_count(void)
{
    return 0U;
}
