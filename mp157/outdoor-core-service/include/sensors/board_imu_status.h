#pragma once

#include <cstddef>
#include <string>

namespace outdoor::sensors {

struct BoardImuStatus {
    bool enabled = false;
    bool seen = false;
    std::string source = "icm20608_iio";
    std::string devicePath;
    std::size_t sampleCount = 0;
    double accelXG = 0.0;
    double accelYG = 0.0;
    double accelZG = 0.0;
    double gyroXDps = 0.0;
    double gyroYDps = 0.0;
    double gyroZDps = 0.0;
    double temperatureCelsius = 0.0;
    std::string lastError;
};

} // namespace outdoor::sensors
