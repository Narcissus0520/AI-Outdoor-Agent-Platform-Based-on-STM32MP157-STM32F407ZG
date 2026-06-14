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
           << ", seq=" << status.lastSequence
           << ", uptime_ms=" << status.uptimeMs
           << ", flags=0x" << std::hex << std::uppercase << status.statusFlags << std::dec
           << ", temp_c=" << std::setprecision(2) << status.temperatureCelsius
           << ", humidity_pct=" << status.humidityPercent
           << ", accel_g=(" << status.accelXG << ", " << status.accelYG << ", " << status.accelZG << ")"
           << ", last_frame=" << status.lastFrameType;
    return stream.str();
}

} // namespace outdoor::mcu
