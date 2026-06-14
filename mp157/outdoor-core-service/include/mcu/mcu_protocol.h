#pragma once

#include <cstddef>
#include <cstdint>

namespace outdoor::mcu {

constexpr std::uint8_t kFrameSof0 = 0xA5;
constexpr std::uint8_t kFrameSof1 = 0x5A;
constexpr std::uint8_t kProtocolVersion = 0x01;
constexpr std::size_t kFrameHeaderSize = 8;
constexpr std::size_t kFrameCrcSize = 2;
constexpr std::size_t kMaxPayloadSize = 64;

enum class McuFrameType : std::uint8_t {
    Heartbeat = 0x01,
    MockSensor = 0x10,
};

constexpr std::size_t kHeartbeatPayloadSize = 6;
constexpr std::size_t kMockSensorPayloadSize = 14;

std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size);
const char* mcuFrameTypeToString(McuFrameType type);

} // namespace outdoor::mcu
