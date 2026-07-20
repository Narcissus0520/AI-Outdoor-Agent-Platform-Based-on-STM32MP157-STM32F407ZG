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
           << ", magnetometer=" << (status.magnetometerSeen ? "true" : "false")
           << ", barometer=" << (status.barometerSeen ? "true" : "false")
           << ", command_ack=" << (status.commandAckSeen ? "true" : "false")
           << ", diagnostics=" << (status.diagnostics.seen ? "true" : "false")
           << ", diagnostics_schema=" << static_cast<unsigned int>(status.diagnostics.schemaVersion)
           << ", seq=" << status.lastSequence
           << ", uptime_ms=" << status.uptimeMs
           << ", flags=0x" << std::hex << std::uppercase << status.statusFlags << std::dec
           << ", command_ack_status=" << static_cast<int>(status.commandAckStatus)
           << ", command_ack_nonce=0x" << std::hex << std::uppercase << status.commandAckNonce << std::dec
           << ", i2c_recovery_count=" << status.diagnostics.i2cRecoveryCount
           << ", fifo_overflow_count=" << status.diagnostics.fifoOverflowCount
           << ", fifo_malformed_count=" << status.diagnostics.fifoMalformedPacketCount
           << ", uart4_rx_drop_count=" << status.diagnostics.uart4RxDropCount
           << ", temp_c=" << std::setprecision(2) << status.temperatureCelsius
           << ", humidity_pct=" << status.humidityPercent
           << ", accel_g=(" << status.accelXG << ", " << status.accelYG << ", " << status.accelZG << ")"
           << ", imu_accel_g=("
           << outdoor::protocol::accelMgToG(status.imuSample.accelXMg) << ", "
           << outdoor::protocol::accelMgToG(status.imuSample.accelYMg) << ", "
           << outdoor::protocol::accelMgToG(status.imuSample.accelZMg) << ")"
           << ", magnetic_ut=("
           << outdoor::protocol::nanoTeslaToMicroTesla(status.magnetometerSample.magneticXNt) << ", "
           << outdoor::protocol::nanoTeslaToMicroTesla(status.magnetometerSample.magneticYNt) << ", "
           << outdoor::protocol::nanoTeslaToMicroTesla(status.magnetometerSample.magneticZNt) << ")"
           << ", compass_valid=" << (status.compassStatus.valid ? "true" : "false")
           << ", compass_heading_deg=" << status.compassStatus.headingDegrees
           << ", compass_quality=" << status.compassStatus.quality
           << ", pressure_pa=" << status.barometerSample.pressurePa
           << ", barometer_temp_c="
           << outdoor::protocol::centiCelsiusToCelsiusBarometer(
                  status.barometerSample.temperatureCentiC)
           << ", last_frame=" << status.lastFrameType;
    return stream.str();
}

} // namespace outdoor::mcu
