#include "sensors/mmc5603_provider_c.h"

#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"

#include <stdint.h>

enum {
    MMC5603_I2C_ADDRESS = 0x30U,
    MMC5603_XOUT0 = 0x00U,
    MMC5603_STATUS1 = 0x18U,
    MMC5603_INTERNAL_CONTROL0 = 0x1BU,
    MMC5603_INTERNAL_CONTROL1 = 0x1CU,
    MMC5603_INTERNAL_CONTROL2 = 0x1DU,
    MMC5603_PRODUCT_ID = 0x39U,
    MMC5603_PRODUCT_ID_VALUE = 0x10U,
    MMC5603_PRODUCT_ID_COMPAT_VALUE = 0x00U,
    MMC5603_CONTROL0_SET = 0x08U,
    MMC5603_CONTROL0_RESET = 0x10U,
    MMC5603_CONTROL0_TAKE_MEAS_M = 0x01U,
    MMC5603_CONTROL1_SW_RESET = 0x80U,
    MMC5603_STATUS_MEAS_M_DONE = 0x40U,
    MMC5603_RAW_OFFSET = 1U << 19,
    MMC5603_NT_PER_LSB_NUMERATOR = 25U,
    MMC5603_NT_PER_LSB_DENOMINATOR = 4U,
    MMC5603_DATA_SIZE = 9U,
    MMC5603_MEASUREMENT_TIMEOUT_MS = 20U,
};

static void delay_ms(uint32_t delay)
{
    const uint32_t start = board_get_tick_ms();
    while ((uint32_t)(board_get_tick_ms() - start) < delay) {
    }
}

static int write_reg(uint8_t reg, uint8_t value)
{
    return board_i2c_mem_write(MMC5603_I2C_ADDRESS, reg, &value, 1U);
}

static int read_reg(uint8_t reg, uint8_t* value)
{
    return board_i2c_mem_read(MMC5603_I2C_ADDRESS, reg, value, 1U);
}

static int read_regs(uint8_t reg, uint8_t* data, uint16_t length)
{
    return board_i2c_mem_read(MMC5603_I2C_ADDRESS, reg, data, length);
}

static int32_t raw_to_nt(uint32_t raw)
{
    const int32_t centered = (int32_t)raw - (int32_t)MMC5603_RAW_OFFSET;
    return (centered * MMC5603_NT_PER_LSB_NUMERATOR) / MMC5603_NT_PER_LSB_DENOMINATOR;
}

int mmc5603_provider_init(mmc5603_provider_t* provider)
{
    uint8_t product_id = 0U;

    if (provider == 0) {
        return -1;
    }

    provider->initialized = 0;
    provider->product_id = 0U;

    if (write_reg(MMC5603_INTERNAL_CONTROL1, MMC5603_CONTROL1_SW_RESET) != 0) {
        return -1;
    }
    delay_ms(20U);

    if (read_reg(MMC5603_PRODUCT_ID, &product_id) != 0) {
        return -1;
    }
    if (product_id != MMC5603_PRODUCT_ID_VALUE &&
        product_id != MMC5603_PRODUCT_ID_COMPAT_VALUE) {
        return -1;
    }

    if (write_reg(MMC5603_INTERNAL_CONTROL0, MMC5603_CONTROL0_SET) != 0) {
        return -1;
    }
    delay_ms(1U);
    if (write_reg(MMC5603_INTERNAL_CONTROL0, MMC5603_CONTROL0_RESET) != 0) {
        return -1;
    }
    delay_ms(1U);
    if (write_reg(MMC5603_INTERNAL_CONTROL2, 0x00U) != 0) {
        return -1;
    }

    provider->product_id = product_id;
    provider->initialized = 1;
    return 0;
}

int mmc5603_provider_read(mmc5603_provider_t* provider,
                          uint32_t uptime_ms,
                          magnetometer_sample_c_t* sample)
{
    uint8_t data[MMC5603_DATA_SIZE] = {0};
    uint8_t status = 0U;
    uint32_t start;
    uint32_t raw_x;
    uint32_t raw_y;
    uint32_t raw_z;

    if (provider == 0 || sample == 0 || provider->initialized == 0) {
        return -1;
    }

    if (write_reg(MMC5603_INTERNAL_CONTROL0, MMC5603_CONTROL0_TAKE_MEAS_M) != 0) {
        return -1;
    }

    start = board_get_tick_ms();
    do {
        if (read_reg(MMC5603_STATUS1, &status) != 0) {
            return -1;
        }
        if ((status & MMC5603_STATUS_MEAS_M_DONE) != 0U) {
            break;
        }
    } while ((uint32_t)(board_get_tick_ms() - start) < MMC5603_MEASUREMENT_TIMEOUT_MS);

    if ((status & MMC5603_STATUS_MEAS_M_DONE) == 0U) {
        return -1;
    }
    if (read_regs(MMC5603_XOUT0, data, MMC5603_DATA_SIZE) != 0) {
        return -1;
    }

    raw_x = ((uint32_t)data[0] << 12U) | ((uint32_t)data[1] << 4U) |
            ((uint32_t)data[6] >> 4U);
    raw_y = ((uint32_t)data[2] << 12U) | ((uint32_t)data[3] << 4U) |
            ((uint32_t)data[7] >> 4U);
    raw_z = ((uint32_t)data[4] << 12U) | ((uint32_t)data[5] << 4U) |
            ((uint32_t)data[8] >> 4U);

    if ((raw_x == 0U && raw_y == 0U && raw_z == 0U) ||
        (raw_x == 0xFFFFFU && raw_y == 0xFFFFFU && raw_z == 0xFFFFFU)) {
        return -1;
    }

    sample->uptime_ms = uptime_ms;
    sample->magnetic_x_nt = raw_to_nt(raw_x);
    sample->magnetic_y_nt = raw_to_nt(raw_y);
    sample->magnetic_z_nt = raw_to_nt(raw_z);
    return 0;
}
