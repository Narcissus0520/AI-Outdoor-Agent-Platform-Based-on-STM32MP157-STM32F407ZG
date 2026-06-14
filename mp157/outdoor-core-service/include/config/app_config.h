#pragma once

#include "log/logger.h"

#include <string>

namespace outdoor::config {

struct AppConfig {
    std::string nmeaInputPath = "data/nmea_sample.txt";
    std::string mcuMockInputPath = "data/mcu_mock_frames.txt";
    std::string statusOutputPath = "runtime/runtime_status.json";
    outdoor::log::LogLevel logLevel = outdoor::log::LogLevel::Info;
};

} // namespace outdoor::config
