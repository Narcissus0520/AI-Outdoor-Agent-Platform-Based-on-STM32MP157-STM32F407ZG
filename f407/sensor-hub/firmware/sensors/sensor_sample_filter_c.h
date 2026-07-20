#pragma once

#include "sensors/barometer_sample_c.h"
#include "sensors/imu_sample_c.h"
#include "sensors/magnetometer_sample_c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t value;
    /* alpha = 1 / (2 ^ smoothing_shift), implemented without floating point. */
    uint8_t smoothing_shift;
    int initialized;
} sensor_iir_filter_c_t;

typedef struct {
    sensor_iir_filter_c_t accel[3];
    sensor_iir_filter_c_t gyro[3];
    sensor_iir_filter_c_t temperature;
} imu_sample_filter_c_t;

typedef struct {
    sensor_iir_filter_c_t magnetic[3];
} magnetometer_sample_filter_c_t;

typedef struct {
    sensor_iir_filter_c_t pressure;
    sensor_iir_filter_c_t temperature;
} barometer_sample_filter_c_t;

void imu_sample_filter_init(imu_sample_filter_c_t* filter);
void imu_sample_filter_reset(imu_sample_filter_c_t* filter);
void imu_sample_filter_apply(imu_sample_filter_c_t* filter, imu_sample_c_t* sample);

void magnetometer_sample_filter_init(magnetometer_sample_filter_c_t* filter);
void magnetometer_sample_filter_reset(magnetometer_sample_filter_c_t* filter);
void magnetometer_sample_filter_apply(magnetometer_sample_filter_c_t* filter,
                                      magnetometer_sample_c_t* sample);

void barometer_sample_filter_init(barometer_sample_filter_c_t* filter);
void barometer_sample_filter_reset(barometer_sample_filter_c_t* filter);
void barometer_sample_filter_apply(barometer_sample_filter_c_t* filter,
                                   barometer_sample_c_t* sample);

#ifdef __cplusplus
}
#endif
