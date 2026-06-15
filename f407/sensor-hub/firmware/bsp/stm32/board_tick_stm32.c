#include "bsp/board_tick.h"

#include "stm32f4xx_hal.h"

uint32_t board_get_tick_ms(void)
{
    return HAL_GetTick();
}
