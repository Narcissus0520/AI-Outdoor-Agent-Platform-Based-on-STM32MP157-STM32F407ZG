#pragma once

#include "sensors/imu_sample_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int initialized;
    uint8_t i2c_address;
} icm42688_provider_t;

int icm42688_provider_init(icm42688_provider_t* provider);
int icm42688_provider_read(icm42688_provider_t* provider, uint32_t uptime_ms, imu_sample_c_t* sample);

#ifdef __cplusplus
}
#endif
