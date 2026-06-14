#include "mcu/mcu_protocol.h"

namespace outdoor::mcu {

std::uint16_t crc16Modbus(const std::uint8_t* data, std::size_t size)
{
    return outdoor::protocol::crc16Modbus(data, size);
}

const char* mcuFrameTypeToString(McuFrameType type)
{
    switch (type) {
    case McuFrameType::Heartbeat:
        return "heartbeat";
    case McuFrameType::MockSensor:
        return "mock_sensor";
    case McuFrameType::SensorImu:
        return "sensor_imu";
    }

    return "unknown";
}

} // namespace outdoor::mcu
