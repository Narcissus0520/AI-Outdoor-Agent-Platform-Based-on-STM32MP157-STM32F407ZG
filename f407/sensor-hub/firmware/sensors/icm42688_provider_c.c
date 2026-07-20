#include "sensors/icm42688_provider_c.h"

#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"
#include "sensors/icm42688_fifo_parser_c.h"

#include <stddef.h>
#include <stdint.h>

enum {
    ICM42688_I2C_ADDRESS_AD0_HIGH = 0x69U,
    ICM42688_I2C_ADDRESS_AD0_LOW = 0x68U,
    ICM42688_DEVICE_CONFIG = 0x11U,
    ICM42688_INT_CONFIG = 0x14U,
    ICM42688_FIFO_CONFIG = 0x16U,
    ICM42688_INT_STATUS = 0x2DU,
    ICM42688_FIFO_COUNTH = 0x2EU,
    ICM42688_FIFO_DATA = 0x30U,
    ICM42688_SIGNAL_PATH_RESET = 0x4BU,
    ICM42688_INTF_CONFIG0 = 0x4CU,
    ICM42688_PWR_MGMT0 = 0x4EU,
    ICM42688_GYRO_CONFIG0 = 0x4FU,
    ICM42688_ACCEL_CONFIG0 = 0x50U,
    ICM42688_GYRO_ACCEL_CONFIG0 = 0x52U,
    ICM42688_FIFO_CONFIG1 = 0x5FU,
    ICM42688_FIFO_CONFIG2 = 0x60U,
    ICM42688_FIFO_CONFIG3 = 0x61U,
    ICM42688_INT_CONFIG1 = 0x64U,
    ICM42688_INT_SOURCE0 = 0x65U,
    ICM42688_WHO_AM_I = 0x75U,
    ICM42688_REG_BANK_SEL = 0x76U,
    ICM42688_WHO_AM_I_VALUE = 0x47U,
    ICM42688_AFS_4G = 0x02U,
    ICM42688_GFS_1000DPS = 0x01U,
    ICM42688_ODR_100HZ = 0x08U,
    ICM42688_UI_FILTER_BW_25HZ = 0x05U,
    ICM42688_INT1_PUSH_PULL_ACTIVE_HIGH_PULSED = 0x03U,
    ICM42688_INT_ASYNC_RESET = 0x10U,
    ICM42688_FIFO_WM_GT_TH = 0x20U,
    ICM42688_FIFO_TEMP_GYRO_ACCEL_EN = 0x07U,
    ICM42688_FIFO_COUNT_RECORDS_BIG_ENDIAN = 0x70U,
    ICM42688_FIFO_COUNT_FORMAT_MASK = 0x70U,
    ICM42688_FIFO_STREAM_MODE = 0x40U,
    ICM42688_FIFO_FLUSH = 0x02U,
    ICM42688_FIFO_WATERMARK_RECORDS = 4U,
    ICM42688_FIFO_THS_INT1_EN = 0x04U,
    ICM42688_FIFO_FULL_INT1_EN = 0x02U,
    ICM42688_GYRO_STARTUP_DELAY_MS = 45U,
    ICM42688_SAMPLE_PERIOD_MS = 10U,
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

static int flush_fifo_at(uint8_t address)
{
    return write_reg_at(address, ICM42688_SIGNAL_PATH_RESET, ICM42688_FIFO_FLUSH);
}

static int initialize_at_address(icm42688_provider_t* provider, uint8_t address)
{
    uint8_t value = 0U;

    provider->init_error_step = 1U;
    if (write_reg_at(address, ICM42688_REG_BANK_SEL, 0x00U) != 0) {
        return -1;
    }

    if (write_reg_at(address, ICM42688_DEVICE_CONFIG, 0x01U) != 0) {
        return -1;
    }
    delay_ms(100U);

    provider->init_error_step = 2U;
    if (read_reg_at(address, ICM42688_WHO_AM_I, &value) != 0 || value != ICM42688_WHO_AM_I_VALUE) {
        return -1;
    }

    if (write_reg_at(address, ICM42688_REG_BANK_SEL, 0x00U) != 0) {
        return -1;
    }

    provider->init_error_step = 3U;
    value = (uint8_t)((ICM42688_AFS_4G << 5U) | ICM42688_ODR_100HZ);
    if (write_reg_at(address, ICM42688_ACCEL_CONFIG0, value) != 0) {
        return -1;
    }

    value = (uint8_t)((ICM42688_GFS_1000DPS << 5U) | ICM42688_ODR_100HZ);
    if (write_reg_at(address, ICM42688_GYRO_CONFIG0, value) != 0) {
        return -1;
    }

    value = (uint8_t)((ICM42688_UI_FILTER_BW_25HZ << 4U) |
                      ICM42688_UI_FILTER_BW_25HZ);
    if (write_reg_at(address, ICM42688_GYRO_ACCEL_CONFIG0, value) != 0) {
        return -1;
    }

    provider->init_error_step = 4U;
    if (write_reg_at(address,
                     ICM42688_INT_CONFIG,
                     ICM42688_INT1_PUSH_PULL_ACTIVE_HIGH_PULSED) != 0) {
        return -1;
    }

    if (read_reg_at(address, ICM42688_INT_CONFIG1, &value) != 0) {
        return -1;
    }
    value = (uint8_t)(value & (uint8_t)~ICM42688_INT_ASYNC_RESET);
    if (write_reg_at(address, ICM42688_INT_CONFIG1, value) != 0) {
        return -1;
    }

    provider->init_error_step = 5U;
    if (read_reg_at(address, ICM42688_INTF_CONFIG0, &value) != 0) {
        return -1;
    }
    value = (uint8_t)((value & (uint8_t)~ICM42688_FIFO_COUNT_FORMAT_MASK) |
                      ICM42688_FIFO_COUNT_RECORDS_BIG_ENDIAN);
    if (write_reg_at(address, ICM42688_INTF_CONFIG0, value) != 0) {
        return -1;
    }

    provider->init_error_step = 6U;
    value = ICM42688_FIFO_WM_GT_TH |
            ICM42688_FIFO_TEMP_GYRO_ACCEL_EN;
    if (write_reg_at(address, ICM42688_FIFO_CONFIG1, value) != 0 ||
        write_reg_at(address,
                     ICM42688_FIFO_CONFIG2,
                     ICM42688_FIFO_WATERMARK_RECORDS) != 0 ||
        write_reg_at(address, ICM42688_FIFO_CONFIG3, 0x00U) != 0 ||
        flush_fifo_at(address) != 0) {
        return -1;
    }
    provider->init_error_step = 7U;
    if (read_reg_at(address, ICM42688_INTF_CONFIG0, &value) != 0 ||
        (value & ICM42688_FIFO_COUNT_FORMAT_MASK) !=
            ICM42688_FIFO_COUNT_RECORDS_BIG_ENDIAN) {
        return -1;
    }
    provider->init_error_step = 8U;
    if (read_reg_at(address, ICM42688_FIFO_CONFIG1, &value) != 0 ||
        value != (ICM42688_FIFO_WM_GT_TH |
                  ICM42688_FIFO_TEMP_GYRO_ACCEL_EN)) {
        return -1;
    }
    provider->init_error_step = 9U;
    if (read_reg_at(address, ICM42688_FIFO_CONFIG2, &value) != 0 ||
        value != ICM42688_FIFO_WATERMARK_RECORDS) {
        return -1;
    }

    provider->init_error_step = 10U;
    if (read_reg_at(address, ICM42688_PWR_MGMT0, &value) != 0) {
        return -1;
    }
    value = (uint8_t)((value & ~0x3FU) | 0x0FU);
    if (write_reg_at(address, ICM42688_PWR_MGMT0, value) != 0) {
        return -1;
    }
    delay_ms(ICM42688_GYRO_STARTUP_DELAY_MS);

    provider->init_error_step = 11U;
    if (write_reg_at(address, ICM42688_FIFO_CONFIG, ICM42688_FIFO_STREAM_MODE) != 0) {
        return -1;
    }
    if (read_reg_at(address, ICM42688_FIFO_CONFIG, &value) != 0 ||
        (value & 0xC0U) != ICM42688_FIFO_STREAM_MODE) {
        return -1;
    }

    provider->init_error_step = 12U;
    if (read_reg_at(address, ICM42688_INT_STATUS, &value) != 0) {
        return -1;
    }
    value = ICM42688_FIFO_THS_INT1_EN | ICM42688_FIFO_FULL_INT1_EN;
    if (write_reg_at(address, ICM42688_INT_SOURCE0, value) != 0) {
        return -1;
    }
    if (read_reg_at(address, ICM42688_INT_SOURCE0, &value) != 0 ||
        value != (ICM42688_FIFO_THS_INT1_EN | ICM42688_FIFO_FULL_INT1_EN)) {
        return -1;
    }

    provider->i2c_address = address;
    provider->initialized = 1;
    provider->init_error_step = 0U;
    return 0;
}

void icm42688_provider_reset(icm42688_provider_t* provider)
{
    if (provider == 0) {
        return;
    }

    provider->initialized = 0;
    provider->i2c_address = 0U;
    provider->fifo_overflow_count = 0U;
    provider->fifo_malformed_packet_count = 0U;
    provider->fifo_empty_event_count = 0U;
    provider->fifo_drain_stall_count = 0U;
    provider->fifo_skipped_packet_count = 0U;
    provider->init_error_step = 0U;
}

int icm42688_provider_init(icm42688_provider_t* provider)
{
    uint8_t high_address_error_step;

    if (provider == 0) {
        return -1;
    }

    provider->initialized = 0;
    provider->i2c_address = 0U;
    provider->init_error_step = 0U;

    if (initialize_at_address(provider, ICM42688_I2C_ADDRESS_AD0_HIGH) == 0) {
        return 0;
    }
    high_address_error_step = provider->init_error_step;
    if (initialize_at_address(provider, ICM42688_I2C_ADDRESS_AD0_LOW) == 0) {
        return 0;
    }
    if (provider->init_error_step <= 2U && high_address_error_step > provider->init_error_step) {
        provider->init_error_step = high_address_error_step;
    }
    return -1;
}

int icm42688_provider_read_fifo(icm42688_provider_t* provider,
                                uint32_t uptime_ms,
                                imu_sample_c_t* samples,
                                size_t capacity,
                                size_t* sample_count)
{
    uint8_t fifo_count[2] = {0U, 0U};
    uint8_t fifo_data[ICM42688_FIFO_BATCH_CAPACITY * ICM42688_FIFO_PACKET_SIZE];
    size_t available_records;
    size_t fifo_length;
    size_t parsed_sample_count = 0U;
    size_t skipped_packet_count = 0U;

    if (provider == 0 || samples == 0 || sample_count == 0 ||
        capacity == 0U ||
        provider->initialized == 0) {
        return -1;
    }
    *sample_count = 0U;

    if (read_regs_at(provider->i2c_address,
                     ICM42688_FIFO_COUNTH,
                     fifo_count,
                     sizeof(fifo_count)) != 0) {
        return -1;
    }

    available_records = ((size_t)fifo_count[0] << 8U) | (size_t)fifo_count[1];
    if (available_records == 0U) {
        ++provider->fifo_empty_event_count;
        return 0;
    }
    if (available_records > capacity ||
        available_records > ICM42688_FIFO_BATCH_CAPACITY) {
        ++provider->fifo_overflow_count;
        (void)flush_fifo_at(provider->i2c_address);
        return ICM42688_PROVIDER_FIFO_OVERFLOW;
    }

    fifo_length = available_records * ICM42688_FIFO_PACKET_SIZE;
    if (board_i2c_mem_read_non_replayable(provider->i2c_address,
                                          ICM42688_FIFO_DATA,
                                          fifo_data,
                                          fifo_length) != 0) {
        (void)flush_fifo_at(provider->i2c_address);
        return -1;
    }

    {
        const int parse_result = icm42688_fifo_parse_stream(fifo_data,
                                                             fifo_length,
                                                             uptime_ms,
                                                             ICM42688_SAMPLE_PERIOD_MS,
                                                             samples,
                                                             capacity,
                                                             &parsed_sample_count,
                                                             &skipped_packet_count);
        if (parse_result != 0) {
            ++provider->fifo_malformed_packet_count;
            (void)flush_fifo_at(provider->i2c_address);
            if (parse_result == ICM42688_FIFO_PARSE_EMPTY_MESSAGE) {
                return ICM42688_PROVIDER_FIFO_EMPTY_PACKET;
            }
            if (parse_result == ICM42688_FIFO_PARSE_HEADER_FORMAT) {
                return ICM42688_PROVIDER_FIFO_HEADER_FORMAT;
            }
            if (parse_result == ICM42688_FIFO_PARSE_OUTPUT_FULL) {
                ++provider->fifo_overflow_count;
                return ICM42688_PROVIDER_FIFO_OVERFLOW;
            }
            return ICM42688_PROVIDER_FIFO_MALFORMED;
        }
    }
    provider->fifo_skipped_packet_count += (uint32_t)skipped_packet_count;
    if (parsed_sample_count == 0U) {
        ++provider->fifo_empty_event_count;
    }

    *sample_count = parsed_sample_count;
    return 0;
}
