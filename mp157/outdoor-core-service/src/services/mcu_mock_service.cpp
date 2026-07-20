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

McuMockService::McuMockService(std::string frameInputPath,
                               outdoor::mcu::McuStatus& status,
                               std::function<bool(std::string&)> statusUpdatedCallback)
    : frameInputPath_(std::move(frameInputPath)),
      status_(status),
      statusUpdatedCallback_(std::move(statusUpdatedCallback)) {}

const char* McuMockService::name() const
{
    return "mcu_mock_service";
}

bool McuMockService::start()
{
    stop();
    stream_.open(frameInputPath_);
    if (!stream_.is_open()) {
        status_.lastError = "failed to open MCU mock frame file: " + frameInputPath_;
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    lineNumber_ = 0;
    outdoor::log::Logger::info("MCU mock frame input opened: " + frameInputPath_);
    return true;
}

outdoor::runtime::ServicePollResult McuMockService::poll()
{
    if (!stream_.is_open()) {
        status_.lastError = "failed to read MCU mock frame file: " + frameInputPath_;
        outdoor::log::Logger::error(status_.lastError);
        return outdoor::runtime::ServicePollResult::Failed;
    }

    std::string line;
    if (!std::getline(stream_, line)) {
        return outdoor::runtime::ServicePollResult::Completed;
    }
    ++lineNumber_;

    std::vector<std::uint8_t> bytes;
    std::string error;
    if (!parseHexLine(line, bytes, error)) {
        if (!error.empty()) {
            std::ostringstream message;
            message << "MCU mock line " << lineNumber_ << " skipped: " << error;
            outdoor::log::Logger::debug(message.str());
        }
        return outdoor::runtime::ServicePollResult::Running;
    }

    outdoor::mcu::McuFrame frame;
    if (!parser_.parseFrame(bytes, frame, error)) {
        status_.lastError = error;
        outdoor::log::Logger::debug("MCU frame skipped: " + error);
        return outdoor::runtime::ServicePollResult::Running;
    }

    if (!parser_.applyFrame(frame, status_, error)) {
        status_.lastError = error;
        outdoor::log::Logger::warn("MCU frame apply failed: " + error);
        return outdoor::runtime::ServicePollResult::Running;
    }

    if (statusUpdatedCallback_) {
        std::string callbackError;
        if (!statusUpdatedCallback_(callbackError)) {
            status_.lastError = "MCU status update callback failed: " + callbackError;
            outdoor::log::Logger::error(status_.lastError);
            return outdoor::runtime::ServicePollResult::Failed;
        }
    }

    outdoor::log::Logger::info(outdoor::mcu::formatMcuStatus(status_));
    return outdoor::runtime::ServicePollResult::Running;
}

void McuMockService::stop()
{
    if (stream_.is_open()) {
        stream_.close();
    }
    stream_.clear();
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
