#pragma once

#include <cstddef>
#include <cstdint>

namespace outdoor::protocol {

constexpr std::uint8_t kFrameSof0 = 0xA5;
constexpr std::uint8_t kFrameSof1 = 0x5A;
constexpr std::uint8_t kProtocolVersion = 0x01;
constexpr std::size_t kFrameHeaderSize = 8;
constexpr std::size_t kFrameCrcSize = 2;
constexpr std::size_t kMaxPayloadSize = 64;

constexpr std::uint8_t MSG_TYPE_HEARTBEAT = 0x01;
constexpr std::uint8_t MSG_TYPE_SENSOR_HUB_DIAGNOSTICS = 0x02;
constexpr std::uint8_t MSG_TYPE_MOCK_SENSOR = 0x10;
constexpr std::uint8_t MSG_TYPE_SENSOR_IMU = 0x11;
constexpr std::uint8_t MSG_TYPE_SENSOR_MAGNETOMETER = 0x12;
constexpr std::uint8_t MSG_TYPE_SENSOR_BAROMETER = 0x13;
constexpr std::uint8_t MSG_TYPE_COMMAND_PING = 0x80;
constexpr std::uint8_t MSG_TYPE_COMMAND_ACK = 0x81;

constexpr std::size_t kSensorHubDiagnosticsLegacyPayloadSize = 44;
constexpr std::size_t kSensorHubDiagnosticsPayloadSize = 48;
constexpr std::uint8_t kSensorHubDiagnosticsExtensionVersion = 1;

inline std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size)
{
    std::uint16_t crc = 0xFFFF;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001U) != 0U) {
                crc = static_cast<std::uint16_t>((crc >> 1U) ^ 0xA001U);
            } else {
                crc = static_cast<std::uint16_t>(crc >> 1U);
            }
        }
    }

    return crc;
}

} // namespace outdoor::protocol
