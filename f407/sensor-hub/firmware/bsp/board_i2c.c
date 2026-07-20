#include "bsp/board_i2c.h"

#if defined(__GNUC__)
#define SENSOR_HUB_WEAK __attribute__((weak))
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define SENSOR_HUB_WEAK __weak
#else
#define SENSOR_HUB_WEAK
#endif

SENSOR_HUB_WEAK int board_i2c_mem_read(uint8_t device_address_7bit, uint8_t register_address, uint8_t* data, size_t length)
{
    (void)device_address_7bit;
    (void)register_address;
    (void)data;
    (void)length;
    return -1;
}

SENSOR_HUB_WEAK int board_i2c_mem_read_non_replayable(uint8_t device_address_7bit,
                                                       uint8_t register_address,
                                                       uint8_t* data,
                                                       size_t length)
{
    (void)device_address_7bit;
    (void)register_address;
    (void)data;
    (void)length;
    return -1;
}

SENSOR_HUB_WEAK int board_i2c_mem_write(uint8_t device_address_7bit,
                                        uint8_t register_address,
                                        const uint8_t* data,
                                        size_t length)
{
    (void)device_address_7bit;
    (void)register_address;
    (void)data;
    (void)length;
    return -1;
}

SENSOR_HUB_WEAK uint32_t board_i2c_recovery_count(void)
{
    return 0U;
}

SENSOR_HUB_WEAK uint32_t board_i2c_transaction_failure_count(void)
{
    return 0U;
}

SENSOR_HUB_WEAK int board_i2c_transaction_failure_active(void)
{
    return 0;
}

SENSOR_HUB_WEAK void board_i2c_get_diagnostics(board_i2c_diagnostics_t* diagnostics)
{
    if (diagnostics != 0) {
        diagnostics->recovery_count = 0U;
        diagnostics->transaction_failure_count = 0U;
        diagnostics->last_hal_error = 0U;
        diagnostics->last_length = 0U;
        diagnostics->last_device_address = 0U;
        diagnostics->last_register_address = 0U;
        diagnostics->last_operation = BOARD_I2C_OPERATION_NONE;
        diagnostics->last_hal_status = 0U;
    }
}

SENSOR_HUB_WEAK void board_i2c_reset_diagnostics(void)
{
}
