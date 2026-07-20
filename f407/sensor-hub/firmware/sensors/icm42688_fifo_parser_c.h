#pragma once

#include "sensors/imu_sample_c.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ICM42688_FIFO_PACKET_SIZE = 16U,
    ICM42688_FIFO_PARSE_EMPTY_MESSAGE = -2,
    ICM42688_FIFO_PARSE_HEADER_FORMAT = -3,
    ICM42688_FIFO_PARSE_INVALID_SAMPLE = -4,
    ICM42688_FIFO_PARSE_TRUNCATED = -5,
    ICM42688_FIFO_PARSE_OUTPUT_FULL = -6,
};

int icm42688_fifo_parse_packet(const uint8_t* packet,
                               uint32_t uptime_ms,
                               imu_sample_c_t* sample);
int icm42688_fifo_parse_stream(const uint8_t* data,
                               size_t length,
                               uint32_t newest_uptime_ms,
                               uint32_t sample_period_ms,
                               imu_sample_c_t* samples,
                               size_t capacity,
                               size_t* sample_count,
                               size_t* skipped_packet_count);

#ifdef __cplusplus
}
#endif
