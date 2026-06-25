#pragma once

#include "protocol/barometer_payload.h"
#include "protocol/imu_payload.h"
#include "protocol/magnetometer_payload.h"

#include <cstdint>
#include <string>

namespace outdoor::mcu {

struct McuStatus {
    bool heartbeatSeen = false;
    bool mockSensorSeen = false;
    bool imuSeen = false;
    bool magnetometerSeen = false;
    bool barometerSeen = false;
    bool commandAckSeen = false;
    std::uint16_t lastSequence = 0;
    std::uint32_t uptimeMs = 0;
    std::uint16_t statusFlags = 0;
    std::uint8_t commandAckRequestType = 0;
    std::uint8_t commandAckStatus = 0;
    std::uint32_t commandAckNonce = 0;
    double temperatureCelsius = 0.0;
    double humidityPercent = 0.0;
    double accelXG = 0.0;
    double accelYG = 0.0;
    double accelZG = 0.0;
    outdoor::protocol::ImuSample imuSample;
    outdoor::protocol::MagnetometerSample magnetometerSample;
    outdoor::protocol::BarometerSample barometerSample;
    std::string lastFrameType = "none";
    std::string lastError;
};

std::string formatMcuStatus(const McuStatus& status);

} // namespace outdoor::mcu
