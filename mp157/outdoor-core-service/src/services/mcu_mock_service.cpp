#include "services/mcu_mock_service.h"

#include "log/logger.h"
#include "mcu/mcu_protocol.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace outdoor::services {

namespace {

int hexValue(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }

    return -1;
}

} // namespace

McuMockService::McuMockService(std::string frameInputPath, outdoor::mcu::McuStatus& status)
    : frameInputPath_(std::move(frameInputPath)),
      status_(status) {}

const char* McuMockService::name() const
{
    return "mcu_mock_service";
}

bool McuMockService::start()
{
    std::ifstream stream(frameInputPath_);
    if (!stream.is_open()) {
        status_.lastError = "failed to open MCU mock frame file: " + frameInputPath_;
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    outdoor::log::Logger::info("MCU mock frame input opened: " + frameInputPath_);
    return true;
}

bool McuMockService::run()
{
    std::ifstream stream(frameInputPath_);
    if (!stream.is_open()) {
        status_.lastError = "failed to read MCU mock frame file: " + frameInputPath_;
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;

        std::vector<std::uint8_t> bytes;
        std::string error;
        if (!parseHexLine(line, bytes, error)) {
            if (!error.empty()) {
                std::ostringstream message;
                message << "MCU mock line " << lineNumber << " skipped: " << error;
                outdoor::log::Logger::debug(message.str());
            }
            continue;
        }

        outdoor::mcu::McuFrame frame;
        if (!parser_.parseFrame(bytes, frame, error)) {
            status_.lastError = error;
            outdoor::log::Logger::debug("MCU frame skipped: " + error);
            continue;
        }

        if (!parser_.applyFrame(frame, status_, error)) {
            status_.lastError = error;
            outdoor::log::Logger::warn("MCU frame apply failed: " + error);
            continue;
        }

        outdoor::log::Logger::info(outdoor::mcu::formatMcuStatus(status_));
    }

    return true;
}

void McuMockService::stop()
{
}

bool McuMockService::parseHexLine(const std::string& line,
                                  std::vector<std::uint8_t>& bytes,
                                  std::string& error) const
{
    bytes.clear();
    error.clear();

    std::string compact;
    for (char ch : line) {
        if (ch == '#') {
            break;
        }
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        compact.push_back(ch);
    }

    if (compact.empty()) {
        return false;
    }

    if ((compact.size() % 2U) != 0U) {
        error = "hex line has odd number of digits";
        return false;
    }

    for (std::size_t index = 0; index < compact.size(); index += 2U) {
        const int high = hexValue(compact[index]);
        const int low = hexValue(compact[index + 1U]);
        if (high < 0 || low < 0) {
            error = "hex line contains non-hex characters";
            bytes.clear();
            return false;
        }
        bytes.push_back(static_cast<std::uint8_t>((high << 4U) | low));
    }

    return true;
}

} // namespace outdoor::services
