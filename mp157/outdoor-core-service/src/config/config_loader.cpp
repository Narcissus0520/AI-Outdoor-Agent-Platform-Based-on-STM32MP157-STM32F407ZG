#include "config/config_loader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

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
