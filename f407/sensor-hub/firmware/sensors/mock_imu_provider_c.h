#pragma once

#include "sensors/imu_sample_c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t period_ms;
    uint32_t tick;
} mock_imu_provider_t;

void mock_imu_provider_init(mock_imu_provider_t* provider, uint32_t period_ms);
void mock_imu_provider_next(mock_imu_provider_t* provider, uint32_t uptime_ms, imu_sample_c_t* sample);

#ifdef __cplusplus
}
#endif
