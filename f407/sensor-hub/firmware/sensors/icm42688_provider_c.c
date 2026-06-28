#include "sensors/icm42688_provider_c.h"

#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"

#include <stdint.h>

enum {
    ICM42688_I2C_ADDRESS_AD0_HIGH = 0x69U,
    ICM42688_I2C_ADDRESS_AD0_LOW = 0x68U,
    ICM42688_DEVICE_CONFIG = 0x11U,
    ICM42688_TEMP_DATA1 = 0x1DU,
    ICM42688_PWR_MGMT0 = 0x4EU,
    ICM42688_GYRO_CONFIG0 = 0x4FU,
    ICM42688_ACCEL_CONFIG0 = 0x50U,
    ICM42688_WHO_AM_I = 0x75U,
    ICM42688_REG_BANK_SEL = 0x76U,
    ICM42688_WHO_AM_I_VALUE = 0x47U,
    ICM42688_AFS_4G = 0x02U,
    ICM42688_GFS_1000DPS = 0x01U,
    ICM42688_ODR_100HZ = 0x08U,
    ICM42688_BURST_SAMPLE_SIZE = 14U,
};

static void delay_ms(uint32_t delay)
{
    const uint32_t start = board_get_tick_ms();
    while ((uint32_t)(board_get_tick_ms() - start) < delay) {
    }
}

static int write_reg_at(uint8_t address, uint8_t reg, uint8_t value)
{
    return board_i2c_mem_write(address, reg, &value, 1U);
}

static int read_reg_at(uint8_t address, uint8_t reg, uint8_t* value)
{
    return board_i2c_mem_read(address, reg, value, 1U);
}

static int read_regs_at(uint8_t address, uint8_t reg, uint8_t* data, uint16_t length)
{
    return board_i2c_mem_read(address, reg, data, length);
}

static int16_t read_i16_be(const uint8_t* data)
{
    return (int16_t)(((uint16_t)data[0] << 8U) | (uint16_t)data[1]);
}

static int16_t accel_raw_to_mg(int16_t raw)
{
    return (int16_t)(((int32_t)raw * 125) / 1024);
}

static int32_t gyro_raw_to_mdps(int16_t raw)
{
    return ((int32_t)raw * 15625) / 512;
}

static int16_t temp_raw_to_centi_c(int16_t raw)
{
    return (int16_t)(2500 + (((int32_t)raw * 10000) / 13248));
}

static int initialize_at_address(icm42688_provider_t* provider, uint8_t address)
{
    uint8_t value = 0U;

    if (write_reg_at(address, ICM42688_REG_BANK_SEL, 0x00U) != 0) {
        return -1;
    }

    if (write_reg_at(address, ICM42688_DEVICE_CONFIG, 0x01U) != 0) {
        return -1;
    }
    delay_ms(100U);

    if (read_reg_at(address, ICM42688_WHO_AM_I, &value) != 0 || value != ICM42688_WHO_AM_I_VALUE) {
        return -1;
    }

    if (write_reg_at(address, ICM42688_REG_BANK_SEL, 0x00U) != 0) {
        return -1;
    }

    value = (uint8_t)((ICM42688_AFS_4G << 5U) | ICM42688_ODR_100HZ);
    if (write_reg_at(address, ICM42688_ACCEL_CONFIG0, value) != 0) {
        return -1;
    }

    value = (uint8_t)((ICM42688_GFS_1000DPS << 5U) | ICM42688_ODR_100HZ);
    if (write_reg_at(address, ICM42688_GYRO_CONFIG0, value) != 0) {
        return -1;
    }

    if (read_reg_at(address, ICM42688_PWR_MGMT0, &value) != 0) {
        return -1;
    }
    value = (uint8_t)((value & ~0x3FU) | 0x0FU);
    if (write_reg_at(address, ICM42688_PWR_MGMT0, value) != 0) {
        return -1;
    }
    delay_ms(1U);

    provider->i2c_address = address;
    provider->initialized = 1;
    return 0;
}

int icm42688_provider_init(icm42688_provider_t* provider)
{
    if (provider == 0) {
        return -1;
    }

    provider->initialized = 0;
    provider->i2c_address = 0U;

    if (initialize_at_address(provider, ICM42688_I2C_ADDRESS_AD0_HIGH) != 0 &&
        initialize_at_address(provider, ICM42688_I2C_ADDRESS_AD0_LOW) != 0) {
        return -1;
    }

    return 0;
}

int icm42688_provider_read(icm42688_provider_t* provider, uint32_t uptime_ms, imu_sample_c_t* sample)
{
    uint8_t buffer[ICM42688_BURST_SAMPLE_SIZE] = {0};
    int16_t temp_raw;
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    if (provider == 0 || sample == 0 || provider->initialized == 0) {
        return -1;
    }

    if (read_regs_at(provider->i2c_address, ICM42688_TEMP_DATA1, buffer, ICM42688_BURST_SAMPLE_SIZE) != 0) {
        return -1;
    }

    temp_raw = read_i16_be(&buffer[0]);
    accel_x_raw = read_i16_be(&buffer[2]);
    accel_y_raw = read_i16_be(&buffer[4]);
    accel_z_raw = read_i16_be(&buffer[6]);
    gyro_x_raw = read_i16_be(&buffer[8]);
    gyro_y_raw = read_i16_be(&buffer[10]);
    gyro_z_raw = read_i16_be(&buffer[12]);

    if ((accel_x_raw == -1 && accel_y_raw == -1 && accel_z_raw == -1) || accel_x_raw == INT16_MIN) {
        return -1;
    }

    sample->uptime_ms = uptime_ms;
    sample->accel_x_mg = accel_raw_to_mg(accel_x_raw);
    sample->accel_y_mg = accel_raw_to_mg(accel_y_raw);
    sample->accel_z_mg = accel_raw_to_mg(accel_z_raw);
    sample->gyro_x_mdps = gyro_raw_to_mdps(gyro_x_raw);
    sample->gyro_y_mdps = gyro_raw_to_mdps(gyro_y_raw);
    sample->gyro_z_mdps = gyro_raw_to_mdps(gyro_z_raw);
    sample->temperature_centi_c = temp_raw_to_centi_c(temp_raw);

    return 0;
}
