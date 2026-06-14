#include "mcu/mcu_frame_parser.h"

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
    case McuFrameType::MockSensor:
        return applyMockSensor(frame, status, error);
    }

    error = "unsupported MCU frame type";
    return false;
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

} // namespace outdoor::mcu
