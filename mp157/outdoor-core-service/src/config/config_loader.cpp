#include "config/config_loader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace outdoor::config {

namespace {

std::string trim(const std::string& value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });

    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (begin >= end) {
        return {};
    }

    return std::string(begin, end);
}

bool parseBool(const std::string& value, bool& parsed)
{
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        parsed = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        parsed = false;
        return true;
    }
    return false;
}

bool parseSize(const std::string& value, std::size_t& parsed)
{
    try {
        std::size_t consumed = 0;
        const auto result = std::stoull(value, &consumed);
        if (consumed != value.size()) {
            return false;
        }
        parsed = static_cast<std::size_t>(result);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseUint32(const std::string& value, std::uint32_t& parsed)
{
    try {
        std::size_t consumed = 0;
        const auto result = std::stoul(value, &consumed);
        if (consumed != value.size()) {
            return false;
        }
        parsed = static_cast<std::uint32_t>(result);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseDouble(const std::string& value, double& parsed)
{
    try {
        std::size_t consumed = 0;
        const double result = std::stod(value, &consumed);
        if (consumed != value.size() || !std::isfinite(result)) {
            return false;
        }
        parsed = result;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

double* compassDoubleTarget(AppConfig& config, const std::string& key)
{
    if (key == "compass_declination_degrees") return &config.compass.declinationDegrees;
    if (key == "compass_hard_iron_x_ut") return &config.compass.hardIronBiasMicroTesla[0];
    if (key == "compass_hard_iron_y_ut") return &config.compass.hardIronBiasMicroTesla[1];
    if (key == "compass_hard_iron_z_ut") return &config.compass.hardIronBiasMicroTesla[2];
    if (key == "compass_soft_iron_00") return &config.compass.softIronMatrix[0];
    if (key == "compass_soft_iron_01") return &config.compass.softIronMatrix[1];
    if (key == "compass_soft_iron_02") return &config.compass.softIronMatrix[2];
    if (key == "compass_soft_iron_10") return &config.compass.softIronMatrix[3];
    if (key == "compass_soft_iron_11") return &config.compass.softIronMatrix[4];
    if (key == "compass_soft_iron_12") return &config.compass.softIronMatrix[5];
    if (key == "compass_soft_iron_20") return &config.compass.softIronMatrix[6];
    if (key == "compass_soft_iron_21") return &config.compass.softIronMatrix[7];
    if (key == "compass_soft_iron_22") return &config.compass.softIronMatrix[8];
    if (key == "compass_minimum_field_ut") return &config.compass.minimumFieldMicroTesla;
    if (key == "compass_maximum_field_ut") return &config.compass.maximumFieldMicroTesla;
    if (key == "compass_minimum_acceleration_g") return &config.compass.minimumAccelerationG;
    if (key == "compass_maximum_acceleration_g") return &config.compass.maximumAccelerationG;
    return nullptr;
}

} // namespace

bool ConfigLoader::load(const std::string& filePath, AppConfig& config, std::string& error) const
{
    std::ifstream stream(filePath);
    if (!stream.is_open()) {
        error = "failed to open config file: " + filePath;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;

        const auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            std::ostringstream message;
            message << "invalid config line " << lineNumber << ": missing '='";
            error = message.str();
            return false;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (key.empty()) {
            std::ostringstream message;
            message << "invalid config line " << lineNumber << ": empty key";
            error = message.str();
            return false;
        }

        if (double* compassValue = compassDoubleTarget(config, key)) {
            if (!parseDouble(value, *compassValue)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber
                        << ": unsupported " << key << " '" << value << "'";
                error = message.str();
                return false;
            }
        } else if (key == "compass_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber
                        << ": unsupported compass_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.compass.enabled = parsed;
        } else if (key == "compass_calibration_valid") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber
                        << ": unsupported compass_calibration_valid '" << value << "'";
                error = message.str();
                return false;
            }
            config.compass.calibrationValid = parsed;
        } else if (key == "compass_maximum_sample_age_ms") {
            std::uint32_t parsed = 0U;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber
                        << ": unsupported compass_maximum_sample_age_ms '" << value << "'";
                error = message.str();
                return false;
            }
            config.compass.maximumSampleAgeMs = parsed;
        } else if (key == "gnss_input_mode") {
            if (value != "file" && value != "serial") {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported gnss_input_mode '" << value << "'";
                error = message.str();
                return false;
            }
            config.gnssInputMode = value;
        } else if (key == "nmea_input_path") {
            config.nmeaInputPath = value;
        } else if (key == "gnss_serial_device") {
            config.gnssSerialDevice = value;
        } else if (key == "gnss_serial_baud") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported gnss_serial_baud '" << value << "'";
                error = message.str();
                return false;
            }
            config.gnssSerialBaud = parsed;
        } else if (key == "gnss_serial_capture_seconds") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported gnss_serial_capture_seconds '" << value << "'";
                error = message.str();
                return false;
            }
            config.gnssSerialCaptureSeconds = parsed;
        } else if (key == "mcu_input_mode") {
            if (value != "mock_file" && value != "serial") {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported mcu_input_mode '" << value << "'";
                error = message.str();
                return false;
            }
            config.mcuInputMode = value;
        } else if (key == "mcu_mock_input_path") {
            config.mcuMockInputPath = value;
        } else if (key == "mcu_serial_device") {
            config.mcuSerialDevice = value;
        } else if (key == "mcu_serial_baud") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported mcu_serial_baud '" << value << "'";
                error = message.str();
                return false;
            }
            config.mcuSerialBaud = parsed;
        } else if (key == "mcu_serial_capture_seconds") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported mcu_serial_capture_seconds '" << value << "'";
                error = message.str();
                return false;
            }
            config.mcuSerialCaptureSeconds = parsed;
        } else if (key == "mcu_command") {
            if (value != "none" && value != "ping") {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported mcu_command '" << value << "'";
                error = message.str();
                return false;
            }
            config.mcuCommand = value;
        } else if (key == "runtime_run_seconds") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported runtime_run_seconds '" << value << "'";
                error = message.str();
                return false;
            }
            config.runtimeRunSeconds = parsed;
        } else if (key == "status_output_path") {
            config.statusOutputPath = value;
        } else if (key == "status_socket_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber
                        << ": unsupported status_socket_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.statusSocketEnabled = parsed;
        } else if (key == "status_socket_path") {
            config.statusSocketPath = value;
        } else if (key == "storage_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported storage_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.storageEnabled = parsed;
        } else if (key == "storage_root_path") {
            config.storageRootPath = value;
        } else if (key == "storage_status_output_path") {
            config.storageStatusOutputPath = value;
        } else if (key == "storage_dashboard_output_path") {
            config.storageDashboardOutputPath = value;
        } else if (key == "storage_log_file_path") {
            config.storageLogFilePath = value;
        } else if (key == "storage_log_max_bytes") {
            std::size_t parsed = 0;
            if (!parseSize(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported storage_log_max_bytes '" << value << "'";
                error = message.str();
                return false;
            }
            config.storageLogMaxBytes = parsed;
        } else if (key == "storage_log_backup_count") {
            std::size_t parsed = 0;
            if (!parseSize(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported storage_log_backup_count '" << value << "'";
                error = message.str();
                return false;
            }
            config.storageLogBackupCount = parsed;
        } else if (key == "history_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported history_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.historyEnabled = parsed;
        } else if (key == "history_output_path") {
            config.historyOutputPath = value;
        } else if (key == "history_flush_interval_ms") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported history_flush_interval_ms '" << value << "'";
                error = message.str();
                return false;
            }
            config.historyFlushIntervalMs = parsed;
        } else if (key == "board_imu_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported board_imu_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.boardImuEnabled = parsed;
        } else if (key == "board_imu_source") {
            if (value != "char_device" && value != "iio" && value != "auto") {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported board_imu_source '" << value << "'";
                error = message.str();
                return false;
            }
            config.boardImuSource = value;
        } else if (key == "board_imu_device_path") {
            config.boardImuDevicePath = value;
        } else if (key == "board_imu_iio_path") {
            config.boardImuIioPath = value;
        } else if (key == "board_imu_sample_count") {
            std::size_t parsed = 0;
            if (!parseSize(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported board_imu_sample_count '" << value << "'";
                error = message.str();
                return false;
            }
            config.boardImuSampleCount = parsed;
        } else if (key == "board_imu_sample_interval_ms") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported board_imu_sample_interval_ms '" << value << "'";
                error = message.str();
                return false;
            }
            config.boardImuSampleIntervalMs = parsed;
        } else if (key == "dashboard_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported dashboard_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.dashboardEnabled = parsed;
        } else if (key == "dashboard_output_path") {
            config.dashboardOutputPath = value;
        } else if (key == "dashboard_output_mode") {
            if (value != "text" && value != "framebuffer" && value != "both") {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported dashboard_output_mode '" << value << "'";
                error = message.str();
                return false;
            }
            config.dashboardOutputMode = value;
        } else if (key == "dashboard_framebuffer_device") {
            config.dashboardFramebufferDevice = value;
        } else if (key == "dashboard_refresh_count") {
            std::size_t parsed = 0;
            if (!parseSize(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported dashboard_refresh_count '" << value << "'";
                error = message.str();
                return false;
            }
            config.dashboardRefreshCount = parsed;
        } else if (key == "dashboard_refresh_interval_ms") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported dashboard_refresh_interval_ms '" << value << "'";
                error = message.str();
                return false;
            }
            config.dashboardRefreshIntervalMs = parsed;
        } else if (key == "launcher_enabled") {
            bool parsed = false;
            if (!parseBool(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported launcher_enabled '" << value << "'";
                error = message.str();
                return false;
            }
            config.launcherEnabled = parsed;
        } else if (key == "launcher_input_device") {
            config.launcherInputDevice = value;
        } else if (key == "launcher_auto_start_seconds") {
            std::uint32_t parsed = 0;
            if (!parseUint32(value, parsed)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported launcher_auto_start_seconds '" << value << "'";
                error = message.str();
                return false;
            }
            config.launcherAutoStartSeconds = parsed;
        } else if (key == "log_level") {
            outdoor::log::LogLevel parsedLevel {};
            if (!outdoor::log::parseLogLevel(value, parsedLevel)) {
                std::ostringstream message;
                message << "invalid config line " << lineNumber << ": unsupported log_level '" << value << "'";
                error = message.str();
                return false;
            }
            config.logLevel = parsedLevel;
        } else {
            std::ostringstream message;
            message << "invalid config line " << lineNumber << ": unknown key '" << key << "'";
            error = message.str();
            return false;
        }
    }

    return outdoor::navigation::validateCompassConfig(config.compass, error);
}

} // namespace outdoor::config
