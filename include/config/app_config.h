#pragma once

#include "log/logger.h"

#include <string>

namespace outdoor::config {

struct AppConfig {
    std::string nmeaInputPath = "data/nmea_sample.txt";
    outdoor::log::LogLevel logLevel = outdoor::log::LogLevel::Info;
};

} // namespace outdoor::config
