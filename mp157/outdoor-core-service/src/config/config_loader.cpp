#include "config/config_loader.h"

#include <algorithm>
#include <cctype>
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

        if (key == "nmea_input_path") {
            config.nmeaInputPath = value;
        } else if (key == "mcu_mock_input_path") {
            config.mcuMockInputPath = value;
        } else if (key == "status_output_path") {
            config.statusOutputPath = value;
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

    error.clear();
    return true;
}

} // namespace outdoor::config
