#pragma once

#include "log/logger.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace outdoor::config {

struct AppConfig {
    std::string gnssInputMode = "file";
    std::string nmeaInputPath = "data/nmea_sample.txt";
    std::string gnssSerialDevice = "/dev/ttySTM2";
    std::uint32_t gnssSerialBaud = 9600;
    std::uint32_t gnssSerialCaptureSeconds = 5;
    std::string mcuInputMode = "mock_file";
    std::string mcuMockInputPath = "data/mcu_mock_frames.txt";
    std::string mcuSerialDevice = "/dev/ttySTM1";
    std::uint32_t mcuSerialBaud = 115200;
    std::uint32_t mcuSerialCaptureSeconds = 5;
    std::string mcuCommand = "none";
    std::string statusOutputPath = "runtime/runtime_status.json";
    bool boardImuEnabled = false;
    std::string boardImuSource = "char_device";
    std::string boardImuDevicePath = "/dev/icm20608";
    std::string boardImuIioPath = "/sys/bus/iio/devices/iio:device0";
    std::size_t boardImuSampleCount = 5;
    std::uint32_t boardImuSampleIntervalMs = 100;
    bool dashboardEnabled = true;
    std::string dashboardOutputPath = "runtime/dashboard.txt";
    std::string dashboardOutputMode = "text";
    std::string dashboardFramebufferDevice = "/dev/fb0";
    std::size_t dashboardRefreshCount = 1;
    std::uint32_t dashboardRefreshIntervalMs = 1000;
    outdoor::log::LogLevel logLevel = outdoor::log::LogLevel::Info;
};

} // namespace outdoor::config
