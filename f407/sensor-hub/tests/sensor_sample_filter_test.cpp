#include "sensors/sensor_sample_filter_c.h"

#include <cassert>
#include <iostream>

namespace {

void testImuFilterStepResponse()
{
    imu_sample_filter_c_t filter{};
    imu_sample_c_t sample{};

    imu_sample_filter_init(&filter);
    sample.accel_x_mg = 1000;
    sample.gyro_x_mdps = 4000;
    sample.temperature_centi_c = 2800;
    imu_sample_filter_apply(&filter, &sample);
    assert(sample.accel_x_mg == 1000);
    assert(sample.gyro_x_mdps == 4000);
    assert(sample.temperature_centi_c == 2800);

    sample.accel_x_mg = 0;
    sample.gyro_x_mdps = 0;
    sample.temperature_centi_c = 2400;
    imu_sample_filter_apply(&filter, &sample);
    assert(sample.accel_x_mg == 750);
    assert(sample.gyro_x_mdps == 3000);
    assert(sample.temperature_centi_c == 2750);

    imu_sample_filter_reset(&filter);
    sample.accel_x_mg = 0;
    imu_sample_filter_apply(&filter, &sample);
    sample.accel_x_mg = -1000;
    imu_sample_filter_apply(&filter, &sample);
    assert(sample.accel_x_mg == -250);
}

void testMagnetometerFilterStepResponse()
{
    magnetometer_sample_filter_c_t filter{};
    magnetometer_sample_c_t sample{};

    magnetometer_sample_filter_init(&filter);
    sample.magnetic_x_nt = 40000;
    magnetometer_sample_filter_apply(&filter, &sample);
    assert(sample.magnetic_x_nt == 40000);

    sample.magnetic_x_nt = 0;
    magnetometer_sample_filter_apply(&filter, &sample);
    assert(sample.magnetic_x_nt == 30000);
}

void testBarometerFilterStepResponse()
{
    barometer_sample_filter_c_t filter{};
    barometer_sample_c_t sample{};

    barometer_sample_filter_init(&filter);
    sample.pressure_pa = 100000U;
    sample.temperature_centi_c = 2800;
    barometer_sample_filter_apply(&filter, &sample);
    assert(sample.pressure_pa == 100000U);

    sample.pressure_pa = 104000U;
    sample.temperature_centi_c = 2400;
    barometer_sample_filter_apply(&filter, &sample);
    assert(sample.pressure_pa == 100500U);
    assert(sample.temperature_centi_c == 2750);
}

} // namespace

int main()
{
    testImuFilterStepResponse();
    testMagnetometerFilterStepResponse();
    testBarometerFilterStepResponse();
    std::cout << "sensor sample filter tests passed\n";
    return 0;
}
