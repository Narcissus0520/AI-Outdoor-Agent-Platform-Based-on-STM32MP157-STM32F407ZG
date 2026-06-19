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
