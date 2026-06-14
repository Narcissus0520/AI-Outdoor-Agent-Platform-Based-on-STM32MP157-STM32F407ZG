#include "mcu/mcu_protocol.h"

namespace outdoor::mcu {

std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size)
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

const char* mcuFrameTypeToString(McuFrameType type)
{
    switch (type) {
    case McuFrameType::Heartbeat:
        return "heartbeat";
    case McuFrameType::MockSensor:
        return "mock_sensor";
    }

    return "unknown";
}

} // namespace outdoor::mcu
