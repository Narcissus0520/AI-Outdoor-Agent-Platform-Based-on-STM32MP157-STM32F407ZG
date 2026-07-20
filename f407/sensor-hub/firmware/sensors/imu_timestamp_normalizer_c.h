#pragma once

#include "sensors/imu_sample_c.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t last_timestamp_ms;
    int initialized;
} imu_timestamp_normalizer_c_t;

void imu_timestamp_normalizer_init(imu_timestamp_normalizer_c_t* normalizer);
int imu_timestamp_normalizer_apply(imu_timestamp_normalizer_c_t* normalizer,
                                   imu_sample_c_t* samples,
                                   size_t sample_count,
                                   uint32_t sample_period_ms);

#ifdef __cplusplus
}
#endif
