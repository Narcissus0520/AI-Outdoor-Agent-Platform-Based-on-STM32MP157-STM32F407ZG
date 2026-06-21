#include "mcu/mcu_status.h"

#include <iomanip>
#include <sstream>

namespace outdoor::mcu {

std::string formatMcuStatus(const McuStatus& status)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << "MCU status: heartbeat=" << (status.heartbeatSeen ? "true" : "false")
           << ", mock_sensor=" << (status.mockSensorSeen ? "true" : "false")
           << ", imu=" << (status.imuSeen ? "true" : "false")
           << ", command_ack=" << (status.commandAckSeen ? "true" : "false")
           << ", seq=" << status.lastSequence
           << ", uptime_ms=" << status.uptimeMs
           << ", flags=0x" << std::hex << std::uppercase << status.statusFlags << std::dec
           << ", command_ack_status=" << static_cast<int>(status.commandAckStatus)
           << ", command_ack_nonce=0x" << std::hex << std::uppercase << status.commandAckNonce << std::dec
           << ", temp_c=" << std::setprecision(2) << status.temperatureCelsius
           << ", humidity_pct=" << status.humidityPercent
           << ", accel_g=(" << status.accelXG << ", " << status.accelYG << ", " << status.accelZG << ")"
           << ", imu_accel_g=("
           << outdoor::protocol::accelMgToG(status.imuSample.accelXMg) << ", "
           << outdoor::protocol::accelMgToG(status.imuSample.accelYMg) << ", "
           << outdoor::protocol::accelMgToG(status.imuSample.accelZMg) << ")"
           << ", last_frame=" << status.lastFrameType;
    return stream.str();
}

} // namespace outdoor::mcu
