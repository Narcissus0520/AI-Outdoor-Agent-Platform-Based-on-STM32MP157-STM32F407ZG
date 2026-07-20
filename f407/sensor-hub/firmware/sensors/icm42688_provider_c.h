#pragma once

#include "sensors/imu_sample_c.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int initialized;
    uint8_t i2c_address;
    uint32_t fifo_overflow_count;
    uint32_t fifo_malformed_packet_count;
    uint32_t fifo_empty_event_count;
    uint32_t fifo_drain_stall_count;
    uint32_t fifo_skipped_packet_count;
    uint8_t init_error_step;
} icm42688_provider_t;

enum {
    ICM42688_FIFO_BATCH_CAPACITY = 16U,
    ICM42688_PROVIDER_FIFO_OVERFLOW = -2,
    ICM42688_PROVIDER_FIFO_MALFORMED = -3,
    ICM42688_PROVIDER_FIFO_DRAIN_STALLED = -4,
    ICM42688_PROVIDER_FIFO_EMPTY_PACKET = -5,
    ICM42688_PROVIDER_FIFO_HEADER_FORMAT = -6,
};

void icm42688_provider_reset(icm42688_provider_t* provider);
int icm42688_provider_init(icm42688_provider_t* provider);
int icm42688_provider_read_fifo(icm42688_provider_t* provider,
                                uint32_t uptime_ms,
                                imu_sample_c_t* samples,
                                size_t capacity,
                                size_t* sample_count);

#ifdef __cplusplus
}
#endif
