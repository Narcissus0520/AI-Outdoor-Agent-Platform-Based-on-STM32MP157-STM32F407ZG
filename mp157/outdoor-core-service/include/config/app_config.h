#pragma once

#include "log/logger.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace outdoor::config {

struct AppConfig {
    std::string nmeaInputPath = "data/nmea_sample.txt";
    std::string mcuMockInputPath = "data/mcu_mock_frames.txt";
    std::string statusOutputPath = "runtime/runtime_status.json";
    bool boardImuEnabled = false;
    std::string boardImuSource = "char_device";
    std::string boardImuDevicePath = "/dev/icm20608";
    std::string boardImuIioPath = "/sys/bus/iio/devices/iio:device0";
    std::size_t boardImuSampleCount = 5;
    std::uint32_t boardImuSampleIntervalMs = 100;
    outdoor::log::LogLevel logLevel = outdoor::log::LogLevel::Info;
};

} // namespace outdoor::config
