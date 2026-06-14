#pragma once

#include "protocol/imu_payload.h"

#include <cstdint>

namespace outdoor::f407::sensors {

class MockImuProvider {
public:
    explicit MockImuProvider(std::uint32_t periodMs = 20);

    outdoor::protocol::ImuSample nextSample();

private:
    std::uint32_t periodMs_;
    std::uint32_t tick_ = 0;
};

} // namespace outdoor::f407::sensors
