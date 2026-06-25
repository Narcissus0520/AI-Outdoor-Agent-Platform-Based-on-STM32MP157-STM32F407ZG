#pragma once

#include "protocol/barometer_payload.h"
#include "protocol/imu_payload.h"
#include "protocol/magnetometer_payload.h"
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
    SensorMagnetometer = outdoor::protocol::MSG_TYPE_SENSOR_MAGNETOMETER,
    SensorBarometer = outdoor::protocol::MSG_TYPE_SENSOR_BAROMETER,
    CommandPing = outdoor::protocol::MSG_TYPE_COMMAND_PING,
    CommandAck = outdoor::protocol::MSG_TYPE_COMMAND_ACK,
};

constexpr std::size_t kHeartbeatPayloadSize = 6;
constexpr std::size_t kMockSensorPayloadSize = 14;
constexpr std::size_t kImuPayloadSize = outdoor::protocol::kImuPayloadSize;
constexpr std::size_t kMagnetometerPayloadSize = outdoor::protocol::kMagnetometerPayloadSize;
constexpr std::size_t kBarometerPayloadSize = outdoor::protocol::kBarometerPayloadSize;
constexpr std::size_t kCommandPingPayloadSize = 4;
constexpr std::size_t kCommandAckPayloadSize = 8;

std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size);
const char* mcuFrameTypeToString(McuFrameType type);

} // namespace outdoor::mcu
