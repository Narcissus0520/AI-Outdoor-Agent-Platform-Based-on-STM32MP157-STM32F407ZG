#include "bsp/board_i2c.h"

#include "i2c.h"

#include <stdint.h>

enum {
    BOARD_I2C_TIMEOUT_MS = 20U,
};

int board_i2c_mem_read(uint8_t device_address_7bit, uint8_t register_address, uint8_t* data, size_t length)
{
    if (data == NULL || length == 0U || length > UINT16_MAX) {
        return -1;
    }

    return HAL_I2C_Mem_Read(&hi2c2,
                            (uint16_t)(device_address_7bit << 1U),
                            register_address,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            (uint16_t)length,
                            BOARD_I2C_TIMEOUT_MS) == HAL_OK
               ? 0
               : -1;
}

int board_i2c_mem_write(uint8_t device_address_7bit, uint8_t register_address, const uint8_t* data, size_t length)
{
    if (data == NULL || length == 0U || length > UINT16_MAX) {
        return -1;
    }

    return HAL_I2C_Mem_Write(&hi2c2,
                             (uint16_t)(device_address_7bit << 1U),
                             register_address,
                             I2C_MEMADD_SIZE_8BIT,
                             (uint8_t*)data,
                             (uint16_t)length,
                             BOARD_I2C_TIMEOUT_MS) == HAL_OK
               ? 0
               : -1;
}
