#include "sensors/mock_imu_provider_c.h"

void mock_imu_provider_init(mock_imu_provider_t* provider, uint32_t period_ms)
{
    if (provider == 0) {
        return;
    }

    provider->period_ms = period_ms;
    provider->tick = 0U;
}

void mock_imu_provider_next(mock_imu_provider_t* provider, uint32_t uptime_ms, imu_sample_c_t* sample)
{
    uint32_t tick = 1U;

    if (sample == 0) {
        return;
    }

    if (provider != 0) {
        provider->tick += 1U;
        tick = provider->tick;
    }

    sample->uptime_ms = uptime_ms;
    sample->accel_x_mg = (int16_t)(12 + (int32_t)(tick % 5U));
    sample->accel_y_mg = (int16_t)(-8 + (int32_t)(tick % 3U));
    sample->accel_z_mg = (int16_t)(1000 + (int32_t)(tick % 7U));
    sample->gyro_x_mdps = 1500 + (int32_t)(tick * 10U);
    sample->gyro_y_mdps = -2200 + (int32_t)(tick * 6U);
    sample->gyro_z_mdps = 320 + (int32_t)(tick * 4U);
    sample->temperature_centi_c = (int16_t)(2530 + (int32_t)(tick % 4U));
}
