#include "sensors/mock_imu_provider.h"

namespace outdoor::f407::sensors {

MockImuProvider::MockImuProvider(std::uint32_t periodMs)
    : periodMs_(periodMs) {}

outdoor::protocol::ImuSample MockImuProvider::nextSample()
{
    ++tick_;

    outdoor::protocol::ImuSample sample;
    sample.uptimeMs = tick_ * periodMs_;
    sample.accelXMg = static_cast<std::int16_t>(12 + static_cast<int>(tick_ % 5U));
    sample.accelYMg = static_cast<std::int16_t>(-8 + static_cast<int>(tick_ % 3U));
    sample.accelZMg = static_cast<std::int16_t>(1000 + static_cast<int>(tick_ % 7U));
    sample.gyroXMdps = 1500 + static_cast<std::int32_t>(tick_ * 10U);
    sample.gyroYMdps = -2200 + static_cast<std::int32_t>(tick_ * 6U);
    sample.gyroZMdps = 320 + static_cast<std::int32_t>(tick_ * 4U);
    sample.temperatureCentiC = static_cast<std::int16_t>(2530 + static_cast<int>(tick_ % 4U));
    return sample;
}

} // namespace outdoor::f407::sensors
