#pragma once

#include <cstddef>
#include <cstdint>

namespace outdoor::protocol {

struct MagnetometerSample {
    std::uint32_t uptimeMs = 0;
    std::int32_t magneticXNt = 0;
    std::int32_t magneticYNt = 0;
    std::int32_t magneticZNt = 0;
};

constexpr std::size_t kMagnetometerPayloadSize = 16;

inline double nanoTeslaToMicroTesla(std::int32_t value)
{
    return static_cast<double>(value) / 1000.0;
}

} // namespace outdoor::protocol
