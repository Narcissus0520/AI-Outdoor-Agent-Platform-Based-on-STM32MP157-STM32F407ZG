#include "mcu/mcu_frame_parser.h"

#include "mcu/imu_payload_parser.h"

#include <sstream>

namespace outdoor::mcu {

namespace {

std::uint16_t readU16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::uint32_t readU32Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::int16_t readI16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::int16_t>(readU16Le(bytes, offset));
}

std::int32_t readI32Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::int32_t>(readU32Le(bytes, offset));
}

} // namespace

bool McuFrameParser::parseFrame(const std::vector<std::uint8_t>& bytes, McuFrame& frame, std::string& error) const
{
    error.clear();
    if (bytes.size() < kFrameHeaderSize + kFrameCrcSize) {
        error = "MCU frame is too short";
        return false;
    }

    if (bytes[0] != kFrameSof0 || bytes[1] != kFrameSof1) {
        error = "MCU frame has invalid SOF";
        return false;
    }

    if (bytes[2] != kProtocolVersion) {
        error = "MCU frame has unsupported protocol version";
        return false;
    }

    const auto payloadSize = readU16Le(bytes, 6);
    if (payloadSize > kMaxPayloadSize) {
        error = "MCU frame payload is too large";
        return false;
    }

    const std::size_t expectedSize = kFrameHeaderSize + payloadSize + kFrameCrcSize;
    if (bytes.size() != expectedSize) {
        error = "MCU frame length does not match payload size";
        return false;
    }

    const std::uint16_t expectedCrc = readU16Le(bytes, kFrameHeaderSize + payloadSize);
    const std::uint16_t actualCrc = crc16Modbus(bytes.data() + 2, kFrameHeaderSize - 2 + payloadSize);
    if (expectedCrc != actualCrc) {
        std::ostringstream message;
        message << "MCU frame CRC mismatch: expected 0x"
                << std::hex << expectedCrc
                << ", calculated 0x" << actualCrc;
        error = message.str();
        return false;
    }

    frame.type = static_cast<McuFrameType>(bytes[3]);
    frame.sequence = readU16Le(bytes, 4);
    frame.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kFrameHeaderSize),
                         bytes.begin() + static_cast<std::ptrdiff_t>(kFrameHeaderSize + payloadSize));
    return true;
}

bool McuFrameParser::applyFrame(const McuFrame& frame, McuStatus& status, std::string& error) const
{
    switch (frame.type) {
    case McuFrameType::Heartbeat:
        return applyHeartbeat(frame, status, error);
    case McuFrameType::SensorHubDiagnostics:
        return applySensorHubDiagnostics(frame, status, error);
    case McuFrameType::MockSensor:
        return applyMockSensor(frame, status, error);
    case McuFrameType::SensorImu:
        return applySensorImu(frame, status, error);
    case McuFrameType::SensorMagnetometer:
        return applySensorMagnetometer(frame, status, error);
    case McuFrameType::SensorBarometer:
        return applySensorBarometer(frame, status, error);
    case McuFrameType::CommandAck:
        return applyCommandAck(frame, status, error);
    case McuFrameType::CommandPing:
        error = "command ping frames are not applied to MP157 status";
        return false;
    }

    error = "unsupported MCU frame type";
    return false;
}

bool McuFrameParser::applySensorHubDiagnostics(const McuFrame& frame,
                                                McuStatus& status,
                                                std::string& error) const
{
    if (frame.payload.size() != kSensorHubDiagnosticsLegacyPayloadSize &&
        frame.payload.size() != kSensorHubDiagnosticsPayloadSize) {
        error = "sensor hub diagnostics payload size is invalid";
        return false;
    }

    const bool hasExtension = frame.payload.size() == kSensorHubDiagnosticsPayloadSize;
    if (hasExtension && frame.payload[43] != kSensorHubDiagnosticsExtensionVersion) {
        error = "sensor hub diagnostics extension version is unsupported";
        return false;
    }

    auto& diagnostics = status.diagnostics;
    diagnostics.seen = true;
    diagnostics.schemaVersion = hasExtension ? frame.payload[43] : 0U;
    diagnostics.uptimeMs = readU32Le(frame.payload, 0);
    diagnostics.i2cRecoveryCount = readU32Le(frame.payload, 4);
    diagnostics.i2cTransactionFailureCount = readU32Le(frame.payload, 8);
    diagnostics.i2cLastHalError = readU32Le(frame.payload, 12);
    diagnostics.fifoOverflowCount = readU32Le(frame.payload, 16);
    diagnostics.fifoMalformedPacketCount = readU32Le(frame.payload, 20);
    diagnostics.fifoEmptyEventCount = readU32Le(frame.payload, 24);
    diagnostics.fifoDrainStallCount = readU32Le(frame.payload, 28);
    diagnostics.fifoSkippedPacketCount = readU32Le(frame.payload, 32);
    diagnostics.i2cLastLength = readU16Le(frame.payload, 36);
    diagnostics.i2cLastDeviceAddress = frame.payload[38];
    diagnostics.i2cLastRegisterAddress = frame.payload[39];
    diagnostics.i2cLastOperation = frame.payload[40];
    diagnostics.i2cLastHalStatus = frame.payload[41];
    diagnostics.icm42688InitErrorStep = frame.payload[42];
    diagnostics.uart4RxDropCount = hasExtension ? readU32Le(frame.payload, 44) : 0U;
    status.lastSequence = frame.sequence;
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    error.clear();
    return true;
}

bool McuFrameParser::applyHeartbeat(const McuFrame& frame, McuStatus& status, std::string& error) const
{
    if (frame.payload.size() != kHeartbeatPayloadSize) {
        error = "heartbeat payload size is invalid";
        return false;
    }

    status.heartbeatSeen = true;
    status.lastSequence = frame.sequence;
    status.uptimeMs = readU32Le(frame.payload, 0);
    status.statusFlags = readU16Le(frame.payload, 4);
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    return true;
}

bool McuFrameParser::applyMockSensor(const McuFrame& frame, McuStatus& status, std::string& error) const
{
    if (frame.payload.size() != kMockSensorPayloadSize) {
        error = "mock sensor payload size is invalid";
        return false;
    }

    status.mockSensorSeen = true;
    status.lastSequence = frame.sequence;
    status.uptimeMs = readU32Le(frame.payload, 0);
    status.temperatureCelsius = static_cast<double>(readI16Le(frame.payload, 4)) / 100.0;
    status.humidityPercent = static_cast<double>(readU16Le(frame.payload, 6)) / 10.0;
    status.accelXG = static_cast<double>(readI16Le(frame.payload, 8)) / 1000.0;
    status.accelYG = static_cast<double>(readI16Le(frame.payload, 10)) / 1000.0;
    status.accelZG = static_cast<double>(readI16Le(frame.payload, 12)) / 1000.0;
    status.lastSequence = frame.sequence;
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    return true;
}

bool McuFrameParser::applySensorImu(const McuFrame& frame, McuStatus& status, std::string& error) const
{
    outdoor::protocol::ImuSample sample;
    ImuPayloadParser parser;
    if (!parser.parse(frame.payload, sample, error)) {
        return false;
    }

    status.imuSeen = true;
    status.lastSequence = frame.sequence;
    status.uptimeMs = sample.uptimeMs;
    status.imuSample = sample;
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    return true;
}

bool McuFrameParser::applySensorMagnetometer(const McuFrame& frame,
                                             McuStatus& status,
                                             std::string& error) const
{
    if (frame.payload.size() != kMagnetometerPayloadSize) {
        error = "magnetometer payload size is invalid";
        return false;
    }

    status.magnetometerSeen = true;
    status.lastSequence = frame.sequence;
    status.uptimeMs = readU32Le(frame.payload, 0);
    status.magnetometerSample.uptimeMs = status.uptimeMs;
    status.magnetometerSample.magneticXNt = readI32Le(frame.payload, 4);
    status.magnetometerSample.magneticYNt = readI32Le(frame.payload, 8);
    status.magnetometerSample.magneticZNt = readI32Le(frame.payload, 12);
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    return true;
}

bool McuFrameParser::applySensorBarometer(const McuFrame& frame,
                                          McuStatus& status,
                                          std::string& error) const
{
    if (frame.payload.size() != kBarometerPayloadSize) {
        error = "barometer payload size is invalid";
        return false;
    }

    status.barometerSeen = true;
    status.lastSequence = frame.sequence;
    status.uptimeMs = readU32Le(frame.payload, 0);
    status.barometerSample.uptimeMs = status.uptimeMs;
    status.barometerSample.pressurePa = readU32Le(frame.payload, 4);
    status.barometerSample.temperatureCentiC = readI16Le(frame.payload, 8);
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    return true;
}

bool McuFrameParser::applyCommandAck(const McuFrame& frame, McuStatus& status, std::string& error) const
{
    if (frame.payload.size() != kCommandAckPayloadSize) {
        error = "command ack payload size is invalid";
        return false;
    }

    status.commandAckSeen = true;
    status.lastSequence = frame.sequence;
    status.commandAckRequestType = frame.payload[0];
    status.commandAckStatus = frame.payload[1];
    status.commandAckNonce = readU32Le(frame.payload, 4);
    status.lastFrameType = mcuFrameTypeToString(frame.type);
    status.lastError.clear();
    return true;
}

} // namespace outdoor::mcu
