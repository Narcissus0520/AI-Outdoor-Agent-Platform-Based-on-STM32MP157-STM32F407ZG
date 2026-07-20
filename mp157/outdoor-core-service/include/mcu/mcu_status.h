#pragma once

#include "protocol/barometer_payload.h"
#include "protocol/imu_payload.h"
#include "protocol/magnetometer_payload.h"
#include "navigation/compass_estimator.h"

#include <cstdint>
#include <string>

namespace outdoor::mcu {

struct SensorHubDiagnostics {
    bool seen = false;
    std::uint8_t schemaVersion = 0;
    std::uint32_t uptimeMs = 0;
    std::uint32_t i2cRecoveryCount = 0;
    std::uint32_t i2cTransactionFailureCount = 0;
    std::uint32_t i2cLastHalError = 0;
    std::uint32_t fifoOverflowCount = 0;
    std::uint32_t fifoMalformedPacketCount = 0;
    std::uint32_t fifoEmptyEventCount = 0;
    std::uint32_t fifoDrainStallCount = 0;
    std::uint32_t fifoSkippedPacketCount = 0;
    std::uint16_t i2cLastLength = 0;
    std::uint8_t i2cLastDeviceAddress = 0;
    std::uint8_t i2cLastRegisterAddress = 0;
    std::uint8_t i2cLastOperation = 0;
    std::uint8_t i2cLastHalStatus = 0;
    std::uint8_t icm42688InitErrorStep = 0;
    std::uint32_t uart4RxDropCount = 0;
};

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
    outdoor::navigation::CompassStatus compassStatus;
    SensorHubDiagnostics diagnostics;
    std::string lastFrameType = "none";
    std::string lastError;
};

std::string formatMcuStatus(const McuStatus& status);

} // namespace outdoor::mcu
