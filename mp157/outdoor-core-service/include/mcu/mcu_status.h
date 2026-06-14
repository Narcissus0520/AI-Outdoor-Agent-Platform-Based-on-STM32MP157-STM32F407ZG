#pragma once

#include "protocol/imu_payload.h"

#include <cstdint>
#include <string>

namespace outdoor::mcu {

struct McuStatus {
    bool heartbeatSeen = false;
    bool mockSensorSeen = false;
    bool imuSeen = false;
    std::uint16_t lastSequence = 0;
    std::uint32_t uptimeMs = 0;
    std::uint16_t statusFlags = 0;
    double temperatureCelsius = 0.0;
    double humidityPercent = 0.0;
    double accelXG = 0.0;
    double accelYG = 0.0;
    double accelZG = 0.0;
    outdoor::protocol::ImuSample imuSample;
    std::string lastFrameType = "none";
    std::string lastError;
};

std::string formatMcuStatus(const McuStatus& status);

} // namespace outdoor::mcu
