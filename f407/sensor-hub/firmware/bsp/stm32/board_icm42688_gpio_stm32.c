#include "bsp/board_icm42688_gpio.h"

#include "main.h"

static volatile uint32_t g_data_ready_event_count;
static uint32_t g_consumed_event_count;

int board_icm42688_data_ready(void)
{
    return HAL_GPIO_ReadPin(ICM42688_INT1_GPIO_Port, ICM42688_INT1_Pin) == GPIO_PIN_SET;
}

int board_icm42688_interrupt_init(void)
{
    g_data_ready_event_count = 0U;
    g_consumed_event_count = 0U;

    __HAL_GPIO_EXTI_CLEAR_IT(ICM42688_INT1_Pin);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3U, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    return 0;
}

int board_icm42688_take_data_ready_event(void)
{
    const uint32_t event_count = g_data_ready_event_count;

    if (event_count == g_consumed_event_count) {
        return 0;
    }

    g_consumed_event_count = event_count;
    return 1;
}

void board_icm42688_discard_data_ready_events(void)
{
    g_consumed_event_count = g_data_ready_event_count;
}

uint32_t board_icm42688_data_ready_event_count(void)
{
    return g_data_ready_event_count;
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(ICM42688_INT1_Pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == ICM42688_INT1_Pin) {
        ++g_data_ready_event_count;
    }
}
