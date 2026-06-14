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
            stream << std::fixed << std::setprecision(3)
                   << "{\n"
                   << "  \"runtime\": {\n"
                   << "    \"state\": \"" << outdoor::runtime::runtimeStateToString(status.state) << "\",\n"
                   << "    \"service_count\": " << status.serviceCount << ",\n"
                   << "    \"started_service_count\": " << status.startedServiceCount << ",\n"
                   << "    \"last_error\": \"" << jsonEscape(status.lastError) << "\"\n"
                   << "  },\n"
                   << "  \"mcu\": {\n"
                   << "    \"heartbeat_seen\": " << (mcu.heartbeatSeen ? "true" : "false") << ",\n"
                   << "    \"mock_sensor_seen\": " << (mcu.mockSensorSeen ? "true" : "false") << ",\n"
                   << "    \"last_sequence\": " << mcu.lastSequence << ",\n"
                   << "    \"uptime_ms\": " << mcu.uptimeMs << ",\n"
                   << "    \"status_flags\": " << mcu.statusFlags << ",\n"
                   << "    \"temperature_celsius\": " << mcu.temperatureCelsius << ",\n"
                   << "    \"humidity_percent\": " << mcu.humidityPercent << ",\n"
                   << "    \"accel_x_g\": " << mcu.accelXG << ",\n"
                   << "    \"accel_y_g\": " << mcu.accelYG << ",\n"
                   << "    \"accel_z_g\": " << mcu.accelZG << ",\n"
                   << "    \"last_frame_type\": \"" << jsonEscape(mcu.lastFrameType) << "\",\n"
                   << "    \"last_error\": \"" << jsonEscape(mcu.lastError) << "\"\n"
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
