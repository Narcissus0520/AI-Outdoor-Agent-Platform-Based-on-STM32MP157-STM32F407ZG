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
    case McuFrameType::SensorHubDiagnostics:
        return "sensor_hub_diagnostics";
    case McuFrameType::MockSensor:
        return "mock_sensor";
    case McuFrameType::SensorImu:
        return "sensor_imu";
    case McuFrameType::SensorMagnetometer:
        return "sensor_magnetometer";
    case McuFrameType::SensorBarometer:
        return "sensor_barometer";
    case McuFrameType::CommandPing:
        return "command_ping";
    case McuFrameType::CommandAck:
        return "command_ack";
    }

    return "unknown";
}

} // namespace outdoor::mcu
