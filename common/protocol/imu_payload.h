#pragma once

#include <cstddef>
#include <cstdint>

namespace outdoor::protocol {

struct ImuSample {
    std::uint32_t uptimeMs = 0;
    std::int16_t accelXMg = 0;
    std::int16_t accelYMg = 0;
    std::int16_t accelZMg = 0;
    std::int32_t gyroXMdps = 0;
    std::int32_t gyroYMdps = 0;
    std::int32_t gyroZMdps = 0;
    std::int16_t temperatureCentiC = 0;
};

constexpr std::size_t kImuPayloadSize = 24;

inline double accelMgToG(std::int16_t value)
{
    return static_cast<double>(value) / 1000.0;
}

inline double gyroMdpsToDps(std::int32_t value)
{
    return static_cast<double>(value) / 1000.0;
}

inline double centiCelsiusToCelsius(std::int16_t value)
{
    return static_cast<double>(value) / 100.0;
}

} // namespace outdoor::protocol
