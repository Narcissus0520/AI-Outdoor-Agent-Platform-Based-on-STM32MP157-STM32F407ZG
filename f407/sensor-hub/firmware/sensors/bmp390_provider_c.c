#include "sensors/bmp390_provider_c.h"

#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

static BMP3_INTF_RET_TYPE bmp390_i2c_read(uint8_t reg_addr,
                                          uint8_t* reg_data,
                                          uint32_t length,
                                          void* intf_ptr)
{
    const uint8_t address = *(const uint8_t*)intf_ptr;
    return board_i2c_mem_read(address, reg_addr, reg_data, (size_t)length) == 0
               ? BMP3_INTF_RET_SUCCESS
               : -1;
}

static BMP3_INTF_RET_TYPE bmp390_i2c_write(uint8_t reg_addr,
                                           const uint8_t* reg_data,
                                           uint32_t length,
                                           void* intf_ptr)
{
    const uint8_t address = *(const uint8_t*)intf_ptr;
    return board_i2c_mem_write(address, reg_addr, reg_data, (size_t)length) == 0
               ? BMP3_INTF_RET_SUCCESS
               : -1;
}

static void bmp390_delay_us(uint32_t period_us, void* intf_ptr)
{
    const uint32_t delay_ms = (period_us + 999U) / 1000U;
    const uint32_t start = board_get_tick_ms();
    (void)intf_ptr;
    while ((uint32_t)(board_get_tick_ms() - start) < delay_ms) {
    }
}

static int initialize_at_address(bmp390_provider_t* provider, uint8_t address)
{
    int8_t result;
    uint32_t settings_select;

    memset(&provider->device, 0, sizeof(provider->device));
    memset(&provider->settings, 0, sizeof(provider->settings));
    provider->i2c_address = address;
    provider->device.intf = BMP3_I2C_INTF;
    provider->device.read = bmp390_i2c_read;
    provider->device.write = bmp390_i2c_write;
    provider->device.delay_us = bmp390_delay_us;
    provider->device.intf_ptr = &provider->i2c_address;

    result = bmp3_init(&provider->device);
    if (result != BMP3_OK || provider->device.chip_id != BMP390_CHIP_ID) {
        return -1;
    }

    provider->settings.press_en = BMP3_ENABLE;
    provider->settings.temp_en = BMP3_ENABLE;
    provider->settings.odr_filter.press_os = BMP3_OVERSAMPLING_2X;
    provider->settings.odr_filter.temp_os = BMP3_OVERSAMPLING_2X;
    provider->settings.odr_filter.odr = BMP3_ODR_25_HZ;
    provider->settings.odr_filter.iir_filter = BMP3_IIR_FILTER_COEFF_3;

    settings_select = BMP3_SEL_PRESS_EN |
                      BMP3_SEL_TEMP_EN |
                      BMP3_SEL_PRESS_OS |
                      BMP3_SEL_TEMP_OS |
                      BMP3_SEL_ODR |
                      BMP3_SEL_IIR_FILTER;
    result = bmp3_set_sensor_settings(settings_select,
                                      &provider->settings,
                                      &provider->device);
    if (result != BMP3_OK) {
        return -1;
    }

    provider->settings.op_mode = BMP3_MODE_NORMAL;
    return bmp3_set_op_mode(&provider->settings, &provider->device) == BMP3_OK ? 0 : -1;
}

int bmp390_provider_init(bmp390_provider_t* provider)
{
    if (provider == 0) {
        return -1;
    }

    memset(provider, 0, sizeof(*provider));
    if (initialize_at_address(provider, BMP3_ADDR_I2C_SEC) != 0 &&
        initialize_at_address(provider, BMP3_ADDR_I2C_PRIM) != 0) {
        return -1;
    }

    provider->initialized = 1;
    return 0;
}

int bmp390_provider_read(bmp390_provider_t* provider,
                         uint32_t uptime_ms,
                         barometer_sample_c_t* sample)
{
    struct bmp3_status status;
    struct bmp3_data data;
    int8_t result;
    uint64_t pressure_pa;

    if (provider == 0 || sample == 0 || provider->initialized == 0) {
        return -1;
    }

    memset(&status, 0, sizeof(status));
    result = bmp3_get_status(&status, &provider->device);
    if (result != BMP3_OK) {
        return -1;
    }
    if (status.sensor.drdy_press == BMP3_DISABLE ||
        status.sensor.drdy_temp == BMP3_DISABLE) {
        return 1;
    }

    result = bmp3_get_sensor_data(BMP3_PRESS_TEMP, &data, &provider->device);
    if (result < BMP3_OK) {
        return -1;
    }

    pressure_pa = data.pressure / 100U;
    if (pressure_pa > UINT32_MAX ||
        data.temperature < INT16_MIN ||
        data.temperature > INT16_MAX) {
        return -1;
    }

    sample->uptime_ms = uptime_ms;
    sample->pressure_pa = (uint32_t)pressure_pa;
    sample->temperature_centi_c = (int16_t)data.temperature;
    return 0;
}
