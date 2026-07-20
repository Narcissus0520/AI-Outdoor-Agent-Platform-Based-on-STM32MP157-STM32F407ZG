#include "sensors/sensor_sample_filter_c.h"

#include <stddef.h>
#include <stdint.h>

enum {
    SENSOR_FILTER_IMU_DYNAMIC_SHIFT = 2U,
    SENSOR_FILTER_MAGNETOMETER_SHIFT = 2U,
    SENSOR_FILTER_SLOW_SHIFT = 3U,
};

static void iir_init(sensor_iir_filter_c_t* filter, uint8_t smoothing_shift)
{
    filter->value = 0;
    filter->smoothing_shift = smoothing_shift;
    filter->initialized = 0;
}

static void iir_reset(sensor_iir_filter_c_t* filter)
{
    filter->value = 0;
    filter->initialized = 0;
}

static int32_t iir_apply(sensor_iir_filter_c_t* filter, int32_t input)
{
    int64_t delta;
    int64_t adjusted_delta;
    int64_t divisor;

    if (filter->initialized == 0) {
        filter->value = input;
        filter->initialized = 1;
        return input;
    }

    divisor = (int64_t)1 << filter->smoothing_shift;
    delta = (int64_t)input - (int64_t)filter->value;
    adjusted_delta = delta;
    if (delta > 0) {
        adjusted_delta += divisor / 2;
    } else if (delta < 0) {
        adjusted_delta -= divisor / 2;
    }

    filter->value = (int32_t)((int64_t)filter->value + adjusted_delta / divisor);
    return filter->value;
}

void imu_sample_filter_init(imu_sample_filter_c_t* filter)
{
    size_t axis;

    if (filter == NULL) {
        return;
    }

    for (axis = 0U; axis < 3U; ++axis) {
        iir_init(&filter->accel[axis], SENSOR_FILTER_IMU_DYNAMIC_SHIFT);
        iir_init(&filter->gyro[axis], SENSOR_FILTER_IMU_DYNAMIC_SHIFT);
    }
    iir_init(&filter->temperature, SENSOR_FILTER_SLOW_SHIFT);
}

void imu_sample_filter_reset(imu_sample_filter_c_t* filter)
{
    size_t axis;

    if (filter == NULL) {
        return;
    }

    for (axis = 0U; axis < 3U; ++axis) {
        iir_reset(&filter->accel[axis]);
        iir_reset(&filter->gyro[axis]);
    }
    iir_reset(&filter->temperature);
}

void imu_sample_filter_apply(imu_sample_filter_c_t* filter, imu_sample_c_t* sample)
{
    if (filter == NULL || sample == NULL) {
        return;
    }

    sample->accel_x_mg = (int16_t)iir_apply(&filter->accel[0], sample->accel_x_mg);
    sample->accel_y_mg = (int16_t)iir_apply(&filter->accel[1], sample->accel_y_mg);
    sample->accel_z_mg = (int16_t)iir_apply(&filter->accel[2], sample->accel_z_mg);
    sample->gyro_x_mdps = iir_apply(&filter->gyro[0], sample->gyro_x_mdps);
    sample->gyro_y_mdps = iir_apply(&filter->gyro[1], sample->gyro_y_mdps);
    sample->gyro_z_mdps = iir_apply(&filter->gyro[2], sample->gyro_z_mdps);
    sample->temperature_centi_c =
        (int16_t)iir_apply(&filter->temperature, sample->temperature_centi_c);
}

void magnetometer_sample_filter_init(magnetometer_sample_filter_c_t* filter)
{
    size_t axis;

    if (filter == NULL) {
        return;
    }

    for (axis = 0U; axis < 3U; ++axis) {
        iir_init(&filter->magnetic[axis], SENSOR_FILTER_MAGNETOMETER_SHIFT);
    }
}

void magnetometer_sample_filter_reset(magnetometer_sample_filter_c_t* filter)
{
    size_t axis;

    if (filter == NULL) {
        return;
    }

    for (axis = 0U; axis < 3U; ++axis) {
        iir_reset(&filter->magnetic[axis]);
    }
}

void magnetometer_sample_filter_apply(magnetometer_sample_filter_c_t* filter,
                                      magnetometer_sample_c_t* sample)
{
    if (filter == NULL || sample == NULL) {
        return;
    }

    sample->magnetic_x_nt = iir_apply(&filter->magnetic[0], sample->magnetic_x_nt);
    sample->magnetic_y_nt = iir_apply(&filter->magnetic[1], sample->magnetic_y_nt);
    sample->magnetic_z_nt = iir_apply(&filter->magnetic[2], sample->magnetic_z_nt);
}

void barometer_sample_filter_init(barometer_sample_filter_c_t* filter)
{
    if (filter == NULL) {
        return;
    }

    iir_init(&filter->pressure, SENSOR_FILTER_SLOW_SHIFT);
    iir_init(&filter->temperature, SENSOR_FILTER_SLOW_SHIFT);
}

void barometer_sample_filter_reset(barometer_sample_filter_c_t* filter)
{
    if (filter == NULL) {
        return;
    }

    iir_reset(&filter->pressure);
    iir_reset(&filter->temperature);
}

void barometer_sample_filter_apply(barometer_sample_filter_c_t* filter,
                                   barometer_sample_c_t* sample)
{
    if (filter == NULL || sample == NULL) {
        return;
    }

    sample->pressure_pa = (uint32_t)iir_apply(&filter->pressure, (int32_t)sample->pressure_pa);
    sample->temperature_centi_c =
        (int16_t)iir_apply(&filter->temperature, sample->temperature_centi_c);
}
