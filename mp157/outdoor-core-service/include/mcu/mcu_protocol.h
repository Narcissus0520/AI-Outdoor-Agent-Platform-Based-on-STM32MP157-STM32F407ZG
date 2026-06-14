#pragma once

#include "protocol/imu_payload.h"
#include "protocol/mcu_protocol.h"

#include <cstddef>
#include <cstdint>

namespace outdoor::mcu {

constexpr std::uint8_t kFrameSof0 = outdoor::protocol::kFrameSof0;
constexpr std::uint8_t kFrameSof1 = outdoor::protocol::kFrameSof1;
constexpr std::uint8_t kProtocolVersion = outdoor::protocol::kProtocolVersion;
constexpr std::size_t kFrameHeaderSize = outdoor::protocol::kFrameHeaderSize;
constexpr std::size_t kFrameCrcSize = outdoor::protocol::kFrameCrcSize;
constexpr std::size_t kMaxPayloadSize = outdoor::protocol::kMaxPayloadSize;

enum class McuFrameType : std::uint8_t {
    Heartbeat = outdoor::protocol::MSG_TYPE_HEARTBEAT,
    MockSensor = outdoor::protocol::MSG_TYPE_MOCK_SENSOR,
    SensorImu = outdoor::protocol::MSG_TYPE_SENSOR_IMU,
};

constexpr std::size_t kHeartbeatPayloadSize = 6;
constexpr std::size_t kMockSensorPayloadSize = 14;
constexpr std::size_t kImuPayloadSize = outdoor::protocol::kImuPayloadSize;

std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size);
const char* mcuFrameTypeToString(McuFrameType type);

} // namespace outdoor::mcu
