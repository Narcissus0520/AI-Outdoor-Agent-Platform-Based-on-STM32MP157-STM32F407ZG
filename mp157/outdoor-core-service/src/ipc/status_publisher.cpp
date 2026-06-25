#include "ipc/status_publisher.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace outdoor::ipc {

namespace {

std::string jsonEscape(const std::string& value)
{
    std::ostringstream stream;
    for (char ch : value) {
        switch (ch) {
        case '\\':
            stream << "\\\\";
            break;
        case '"':
            stream << "\\\"";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            stream << ch;
            break;
        }
    }
    return stream.str();
}

} // namespace

StatusPublisher::StatusPublisher(std::string outputPath)
    : outputPath_(std::move(outputPath)) {}

bool StatusPublisher::publish(const outdoor::runtime::RuntimeStatus& status, std::string& error) const
{
    namespace fs = std::filesystem;

    try {
        const fs::path path(outputPath_);
        const fs::path parent = path.parent_path();
        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        const fs::path tempPath = path.string() + ".tmp";
        {
            std::ofstream stream(tempPath, std::ios::trunc);
            if (!stream.is_open()) {
                error = "failed to open temporary status output path: " + tempPath.string();
                return false;
            }

            const auto& mcu = status.mcuStatus;
            const auto& imu = mcu.imuSample;
            const auto& gnss = status.gnssStatus;
            const auto& fix = gnss.fix;
            const auto& boardImu = status.boardImuStatus;
            stream << std::fixed << std::setprecision(3)
                   << "{\n"
                   << "  \"runtime\": {\n"
                   << "    \"state\": \"" << outdoor::runtime::runtimeStateToString(status.state) << "\",\n"
                   << "    \"service_count\": " << status.serviceCount << ",\n"
                   << "    \"started_service_count\": " << status.startedServiceCount << ",\n"
                   << "    \"last_error\": \"" << jsonEscape(status.lastError) << "\"\n"
                   << "  },\n"
                   << "  \"gnss\": {\n"
                   << "    \"enabled\": " << (gnss.enabled ? "true" : "false") << ",\n"
                   << "    \"seen\": " << (gnss.seen ? "true" : "false") << ",\n"
                   << "    \"source\": \"" << jsonEscape(gnss.source) << "\",\n"
                   << "    \"valid_sentence_count\": " << gnss.validSentenceCount << ",\n"
                   << "    \"skipped_sentence_count\": " << gnss.skippedSentenceCount << ",\n"
                   << "    \"last_sentence_type\": \"" << jsonEscape(gnss.lastSentenceType) << "\",\n"
                   << "    \"fix_valid\": " << (fix.valid ? "true" : "false") << ",\n"
                   << "    \"latitude_degrees\": " << std::setprecision(6) << fix.latitudeDegrees << ",\n"
                   << "    \"longitude_degrees\": " << fix.longitudeDegrees << ",\n"
                   << std::setprecision(3)
                   << "    \"altitude_meters\": " << fix.altitudeMeters << ",\n"
                   << "    \"speed_knots\": " << fix.speedKnots << ",\n"
                   << "    \"speed_kmh\": " << fix.speedKmh << ",\n"
                   << "    \"course_degrees\": " << fix.courseDegrees << ",\n"
                   << "    \"satellite_count\": " << fix.satelliteCount << ",\n"
                   << "    \"satellites_in_view\": " << fix.satellitesInView << ",\n"
                   << "    \"fix_quality\": " << fix.fixQuality << ",\n"
                   << "    \"fix_type\": " << fix.fixType << ",\n"
                   << "    \"hdop\": " << fix.hdop << ",\n"
                   << "    \"pdop\": " << fix.pdop << ",\n"
                   << "    \"vdop\": " << fix.vdop << ",\n"
                   << "    \"utc_time\": \"" << jsonEscape(fix.utcTime) << "\",\n"
                   << "    \"last_error\": \"" << jsonEscape(gnss.lastError) << "\"\n"
                   << "  },\n"
                   << "  \"mcu\": {\n"
                   << "    \"heartbeat_seen\": " << (mcu.heartbeatSeen ? "true" : "false") << ",\n"
                   << "    \"mock_sensor_seen\": " << (mcu.mockSensorSeen ? "true" : "false") << ",\n"
                   << "    \"imu_seen\": " << (mcu.imuSeen ? "true" : "false") << ",\n"
                   << "    \"magnetometer_seen\": " << (mcu.magnetometerSeen ? "true" : "false") << ",\n"
                   << "    \"barometer_seen\": " << (mcu.barometerSeen ? "true" : "false") << ",\n"
                   << "    \"command_ack_seen\": " << (mcu.commandAckSeen ? "true" : "false") << ",\n"
                   << "    \"last_sequence\": " << mcu.lastSequence << ",\n"
                   << "    \"uptime_ms\": " << mcu.uptimeMs << ",\n"
                   << "    \"status_flags\": " << mcu.statusFlags << ",\n"
                   << "    \"command_ack_request_type\": " << static_cast<int>(mcu.commandAckRequestType) << ",\n"
                   << "    \"command_ack_status\": " << static_cast<int>(mcu.commandAckStatus) << ",\n"
                   << "    \"command_ack_nonce\": " << mcu.commandAckNonce << ",\n"
                   << "    \"temperature_celsius\": " << mcu.temperatureCelsius << ",\n"
                   << "    \"humidity_percent\": " << mcu.humidityPercent << ",\n"
                   << "    \"accel_x_g\": " << mcu.accelXG << ",\n"
                   << "    \"accel_y_g\": " << mcu.accelYG << ",\n"
                   << "    \"accel_z_g\": " << mcu.accelZG << ",\n"
                   << "    \"last_frame_type\": \"" << jsonEscape(mcu.lastFrameType) << "\",\n"
                   << "    \"last_error\": \"" << jsonEscape(mcu.lastError) << "\"\n"
                   << "  },\n"
                   << "  \"barometer\": {\n"
                   << "    \"seen\": " << (mcu.barometerSeen ? "true" : "false") << ",\n"
                   << "    \"uptime_ms\": " << mcu.barometerSample.uptimeMs << ",\n"
                   << "    \"pressure_pa\": " << mcu.barometerSample.pressurePa << ",\n"
                   << "    \"temperature_celsius\": "
                   << outdoor::protocol::centiCelsiusToCelsiusBarometer(
                          mcu.barometerSample.temperatureCentiC) << "\n"
                   << "  },\n"
                   << "  \"magnetometer\": {\n"
                   << "    \"seen\": " << (mcu.magnetometerSeen ? "true" : "false") << ",\n"
                   << "    \"uptime_ms\": " << mcu.magnetometerSample.uptimeMs << ",\n"
                   << "    \"magnetic_x_ut\": "
                   << outdoor::protocol::nanoTeslaToMicroTesla(mcu.magnetometerSample.magneticXNt) << ",\n"
                   << "    \"magnetic_y_ut\": "
                   << outdoor::protocol::nanoTeslaToMicroTesla(mcu.magnetometerSample.magneticYNt) << ",\n"
                   << "    \"magnetic_z_ut\": "
                   << outdoor::protocol::nanoTeslaToMicroTesla(mcu.magnetometerSample.magneticZNt) << "\n"
                   << "  },\n"
                   << "  \"imu\": {\n"
                   << "    \"seen\": " << (mcu.imuSeen ? "true" : "false") << ",\n"
                   << "    \"uptime_ms\": " << imu.uptimeMs << ",\n"
                   << "    \"accel_x_g\": " << outdoor::protocol::accelMgToG(imu.accelXMg) << ",\n"
                   << "    \"accel_y_g\": " << outdoor::protocol::accelMgToG(imu.accelYMg) << ",\n"
                   << "    \"accel_z_g\": " << outdoor::protocol::accelMgToG(imu.accelZMg) << ",\n"
                   << "    \"gyro_x_dps\": " << outdoor::protocol::gyroMdpsToDps(imu.gyroXMdps) << ",\n"
                   << "    \"gyro_y_dps\": " << outdoor::protocol::gyroMdpsToDps(imu.gyroYMdps) << ",\n"
                   << "    \"gyro_z_dps\": " << outdoor::protocol::gyroMdpsToDps(imu.gyroZMdps) << ",\n"
                   << "    \"temperature_celsius\": "
                   << outdoor::protocol::centiCelsiusToCelsius(imu.temperatureCentiC) << "\n"
                   << "  },\n"
                   << "  \"board_imu\": {\n"
                   << "    \"enabled\": " << (boardImu.enabled ? "true" : "false") << ",\n"
                   << "    \"seen\": " << (boardImu.seen ? "true" : "false") << ",\n"
                   << "    \"source\": \"" << jsonEscape(boardImu.source) << "\",\n"
                   << "    \"device_path\": \"" << jsonEscape(boardImu.devicePath) << "\",\n"
                   << "    \"sample_count\": " << boardImu.sampleCount << ",\n"
                   << "    \"accel_x_g\": " << boardImu.accelXG << ",\n"
                   << "    \"accel_y_g\": " << boardImu.accelYG << ",\n"
                   << "    \"accel_z_g\": " << boardImu.accelZG << ",\n"
                   << "    \"gyro_x_dps\": " << boardImu.gyroXDps << ",\n"
                   << "    \"gyro_y_dps\": " << boardImu.gyroYDps << ",\n"
                   << "    \"gyro_z_dps\": " << boardImu.gyroZDps << ",\n"
                   << "    \"temperature_celsius\": " << boardImu.temperatureCelsius << ",\n"
                   << "    \"last_error\": \"" << jsonEscape(boardImu.lastError) << "\"\n"
                   << "  }\n"
                   << "}\n";
        }

        if (fs::exists(path)) {
            fs::remove(path);
        }
        fs::rename(tempPath, path);
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        return false;
    }

    error.clear();
    return true;
}

const std::string& StatusPublisher::outputPath() const
{
    return outputPath_;
}

} // namespace outdoor::ipc
