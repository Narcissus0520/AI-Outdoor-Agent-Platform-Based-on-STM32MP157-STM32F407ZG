#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t uptime_ms;
    int16_t accel_x_mg;
    int16_t accel_y_mg;
    int16_t accel_z_mg;
    int32_t gyro_x_mdps;
    int32_t gyro_y_mdps;
    int32_t gyro_z_mdps;
    int16_t temperature_centi_c;
} imu_sample_c_t;

typedef struct {
    uint32_t period_ms;
    uint32_t tick;
} mock_imu_provider_t;

void mock_imu_provider_init(mock_imu_provider_t* provider, uint32_t period_ms);
void mock_imu_provider_next(mock_imu_provider_t* provider, uint32_t uptime_ms, imu_sample_c_t* sample);

#ifdef __cplusplus
}
#endif
