#pragma once

#include <cstddef>
#include <cstdint>

namespace outdoor::protocol {

struct BarometerSample {
    std::uint32_t uptimeMs = 0;
    std::uint32_t pressurePa = 0;
    std::int16_t temperatureCentiC = 0;
};

constexpr std::size_t kBarometerPayloadSize = 10;

inline double centiCelsiusToCelsiusBarometer(std::int16_t value)
{
    return static_cast<double>(value) / 100.0;
}

} // namespace outdoor::protocol
