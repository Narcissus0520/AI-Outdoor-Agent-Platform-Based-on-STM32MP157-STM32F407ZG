#include "bsp/board_icm42688_gpio.h"

#include "main.h"

int board_icm42688_data_ready(void)
{
    return HAL_GPIO_ReadPin(ICM42688_INT1_GPIO_Port, ICM42688_INT1_Pin) == GPIO_PIN_SET;
}
