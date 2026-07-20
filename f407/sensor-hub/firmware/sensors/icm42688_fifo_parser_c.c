#include "sensors/icm42688_fifo_parser_c.h"

#include <stdint.h>

enum {
    ICM42688_FIFO_HEADER_FORMAT_MASK = 0xFCU,
    ICM42688_FIFO_HEADER_ACCEL_GYRO_16BIT = 0x68U,
    ICM42688_FIFO_HEADER_MESSAGE = 0x80U,
    ICM42688_FIFO_HEADER_ACCEL = 0x40U,
    ICM42688_FIFO_HEADER_GYRO = 0x20U,
    ICM42688_FIFO_HEADER_HIRES = 0x10U,
    ICM42688_FIFO_SINGLE_SENSOR_PACKET_SIZE = 8U,
};

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

static int16_t fifo_temp_raw_to_centi_c(int8_t raw)
{
    return (int16_t)(2500 + (((int32_t)raw * 10000) / 207));
}

int icm42688_fifo_parse_packet(const uint8_t* packet,
                               uint32_t uptime_ms,
                               imu_sample_c_t* sample)
{
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    if (packet == 0 || sample == 0) {
        return -1;
    }
    if ((packet[0] & 0x80U) != 0U) {
        return ICM42688_FIFO_PARSE_EMPTY_MESSAGE;
    }
    if ((packet[0] & ICM42688_FIFO_HEADER_FORMAT_MASK) !=
        ICM42688_FIFO_HEADER_ACCEL_GYRO_16BIT) {
        return ICM42688_FIFO_PARSE_HEADER_FORMAT;
    }

    accel_x_raw = read_i16_be(&packet[1]);
    accel_y_raw = read_i16_be(&packet[3]);
    accel_z_raw = read_i16_be(&packet[5]);
    gyro_x_raw = read_i16_be(&packet[7]);
    gyro_y_raw = read_i16_be(&packet[9]);
    gyro_z_raw = read_i16_be(&packet[11]);

    if (accel_x_raw == INT16_MIN || accel_y_raw == INT16_MIN ||
        accel_z_raw == INT16_MIN || gyro_x_raw == INT16_MIN ||
        gyro_y_raw == INT16_MIN || gyro_z_raw == INT16_MIN) {
        return ICM42688_FIFO_PARSE_INVALID_SAMPLE;
    }

    sample->uptime_ms = uptime_ms;
    sample->accel_x_mg = accel_raw_to_mg(accel_x_raw);
    sample->accel_y_mg = accel_raw_to_mg(accel_y_raw);
    sample->accel_z_mg = accel_raw_to_mg(accel_z_raw);
    sample->gyro_x_mdps = gyro_raw_to_mdps(gyro_x_raw);
    sample->gyro_y_mdps = gyro_raw_to_mdps(gyro_y_raw);
    sample->gyro_z_mdps = gyro_raw_to_mdps(gyro_z_raw);
    sample->temperature_centi_c = fifo_temp_raw_to_centi_c((int8_t)packet[13]);
    return 0;
}

int icm42688_fifo_parse_stream(const uint8_t* data,
                               size_t length,
                               uint32_t newest_uptime_ms,
                               uint32_t sample_period_ms,
                               imu_sample_c_t* samples,
                               size_t capacity,
                               size_t* sample_count,
                               size_t* skipped_packet_count)
{
    size_t offset = 0U;
    size_t parsed_count = 0U;
    size_t skipped_count = 0U;
    size_t sensor_packet_count = 0U;
    size_t index;

    if (data == 0 || samples == 0 || sample_count == 0 ||
        skipped_packet_count == 0 || capacity == 0U) {
        return -1;
    }

    while (offset < length) {
        const uint8_t header = data[offset];
        size_t packet_size;

        if ((header & ICM42688_FIFO_HEADER_MESSAGE) != 0U) {
            /* HEADER_MSG=1 is the device's end-of-valid-data / empty marker. */
            ++skipped_count;
            break;
        }
        if ((header & ICM42688_FIFO_HEADER_HIRES) != 0U) {
            return ICM42688_FIFO_PARSE_HEADER_FORMAT;
        }

        if ((header & (ICM42688_FIFO_HEADER_ACCEL | ICM42688_FIFO_HEADER_GYRO)) ==
            (ICM42688_FIFO_HEADER_ACCEL | ICM42688_FIFO_HEADER_GYRO)) {
            packet_size = ICM42688_FIFO_PACKET_SIZE;
        } else if ((header & (ICM42688_FIFO_HEADER_ACCEL | ICM42688_FIFO_HEADER_GYRO)) != 0U) {
            packet_size = ICM42688_FIFO_SINGLE_SENSOR_PACKET_SIZE;
        } else {
            return ICM42688_FIFO_PARSE_HEADER_FORMAT;
        }

        if (offset + packet_size > length) {
            return ICM42688_FIFO_PARSE_TRUNCATED;
        }

        if (packet_size == ICM42688_FIFO_PACKET_SIZE) {
            int result;

            if (parsed_count >= capacity) {
                return ICM42688_FIFO_PARSE_OUTPUT_FULL;
            }
            result = icm42688_fifo_parse_packet(&data[offset], 0U, &samples[parsed_count]);
            if (result != 0) {
                return result;
            }
            samples[parsed_count].uptime_ms = (uint32_t)sensor_packet_count;
            ++parsed_count;
        } else {
            ++skipped_count;
        }
        ++sensor_packet_count;
        offset += packet_size;
    }

    for (index = 0U; index < parsed_count; ++index) {
        const uint32_t packet_ordinal = samples[index].uptime_ms;
        const uint32_t age_ms =
            ((uint32_t)sensor_packet_count - 1U - packet_ordinal) * sample_period_ms;
        samples[index].uptime_ms =
            newest_uptime_ms >= age_ms ? newest_uptime_ms - age_ms : 0U;
    }

    *sample_count = parsed_count;
    *skipped_packet_count = skipped_count;
    return 0;
}
