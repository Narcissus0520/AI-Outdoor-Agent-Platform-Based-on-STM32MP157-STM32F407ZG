#include "services/mcu_serial_service.h"

#include "log/logger.h"

#include <array>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace outdoor::services {

namespace {

#ifndef _WIN32
bool baudToSpeed(std::uint32_t baudRate, speed_t& speed)
{
    switch (baudRate) {
    case 9600:
        speed = B9600;
        return true;
    case 19200:
        speed = B19200;
        return true;
    case 38400:
        speed = B38400;
        return true;
    case 57600:
        speed = B57600;
        return true;
    case 115200:
        speed = B115200;
        return true;
    case 230400:
        speed = B230400;
        return true;
    default:
        return false;
    }
}

std::string errnoMessage(const std::string& prefix)
{
    return prefix + ": " + std::strerror(errno);
}
#endif

} // namespace

McuSerialService::McuSerialService(std::string devicePath,
                                   std::uint32_t baudRate,
                                   std::uint32_t captureSeconds,
                                   outdoor::mcu::McuCommand command,
                                   outdoor::mcu::McuStatus& status,
                                   std::function<bool(std::string&)> statusUpdatedCallback)
    : devicePath_(std::move(devicePath)),
      baudRate_(baudRate),
      captureSeconds_(captureSeconds),
      command_(command),
      status_(status),
      statusUpdatedCallback_(std::move(statusUpdatedCallback))
{
}

McuSerialService::~McuSerialService()
{
    closePort();
}

const char* McuSerialService::name() const
{
    return "mcu_serial_service";
}

bool McuSerialService::start()
{
#ifdef _WIN32
    status_.lastError = "MCU serial input is only supported on Linux targets";
    outdoor::log::Logger::error(status_.lastError);
    return false;
#else
    fd_ = ::open(devicePath_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        status_.lastError = errnoMessage("failed to open MCU serial device " + devicePath_);
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    std::string error;
    if (!configurePort(error)) {
        status_.lastError = error;
        outdoor::log::Logger::error(status_.lastError);
        closePort();
        return false;
    }

    streamDecoder_.reset();
    if (!prepareConfiguredCommand(error)) {
        status_.lastError = error;
        outdoor::log::Logger::error(status_.lastError);
        closePort();
        return false;
    }
    deadlineEnabled_ = captureSeconds_ > 0U;
    if (deadlineEnabled_) {
        deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(captureSeconds_);
    }
    outdoor::log::Logger::info("MCU serial input opened: " + devicePath_);
    return true;
#endif
}

outdoor::runtime::ServicePollResult McuSerialService::poll()
{
#ifdef _WIN32
    return outdoor::runtime::ServicePollResult::Failed;
#else
    if (fd_ < 0) {
        status_.lastError = "MCU serial device is not open";
        outdoor::log::Logger::error(status_.lastError);
        return outdoor::runtime::ServicePollResult::Failed;
    }

    if (!commandSent_) {
        const auto commandResult = pollConfiguredCommand();
        if (commandResult != outdoor::runtime::ServicePollResult::Completed) {
            return commandResult;
        }
    }

    if (deadlineEnabled_ && std::chrono::steady_clock::now() >= deadline_) {
        if (!status_.heartbeatSeen && !status_.imuSeen) {
            status_.lastError = "MCU serial capture completed without heartbeat or IMU frames";
            outdoor::log::Logger::warn(status_.lastError);
            return outdoor::runtime::ServicePollResult::Failed;
        }

        if (command_ == outdoor::mcu::McuCommand::Ping && !status_.commandAckSeen) {
            status_.lastError = "MCU serial capture completed without command ack";
            outdoor::log::Logger::warn(status_.lastError);
            return outdoor::runtime::ServicePollResult::Failed;
        }
        return outdoor::runtime::ServicePollResult::Completed;
    }

    std::array<std::uint8_t, 256U> buffer {};
    const ssize_t readSize = ::read(fd_, buffer.data(), buffer.size());
    if (readSize < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return outdoor::runtime::ServicePollResult::Running;
        }
        status_.lastError = errnoMessage("failed to read MCU serial device " + devicePath_);
        outdoor::log::Logger::error(status_.lastError);
        return outdoor::runtime::ServicePollResult::Failed;
    }

    if (readSize == 0) {
        return outdoor::runtime::ServicePollResult::Running;
    }

    std::string error;
    std::vector<std::vector<std::uint8_t>> rawFrames;
    if (!streamDecoder_.feed(buffer.data(), static_cast<std::size_t>(readSize), rawFrames, error)) {
        status_.lastError = error;
        outdoor::log::Logger::warn(error);
        return outdoor::runtime::ServicePollResult::Running;
    }
    if (!error.empty()) {
        status_.lastError = error;
        outdoor::log::Logger::debug(error);
    }

    for (const auto& rawFrame : rawFrames) {
        outdoor::mcu::McuFrame frame;
        if (!parser_.parseFrame(rawFrame, frame, error)) {
            status_.lastError = error;
            outdoor::log::Logger::debug("MCU serial frame skipped: " + error);
            continue;
        }

        if (!parser_.applyFrame(frame, status_, error)) {
            status_.lastError = error;
            outdoor::log::Logger::warn("MCU serial frame apply failed: " + error);
            continue;
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
    }

    return outdoor::runtime::ServicePollResult::Running;
#endif
}

void McuSerialService::stop()
{
    closePort();
}

bool McuSerialService::configurePort(std::string& error)
{
#ifdef _WIN32
    error = "MCU serial input is only supported on Linux targets";
    return false;
#else
    speed_t speed {};
    if (!baudToSpeed(baudRate_, speed)) {
        error = "unsupported MCU serial baud rate: " + std::to_string(baudRate_);
        return false;
    }

    termios options {};
    if (::tcgetattr(fd_, &options) != 0) {
        error = errnoMessage("failed to read MCU serial attributes");
        return false;
    }

    ::cfmakeraw(&options);
    if (::cfsetispeed(&options, speed) != 0 || ::cfsetospeed(&options, speed) != 0) {
        error = errnoMessage("failed to set MCU serial baud rate");
        return false;
    }

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= static_cast<tcflag_t>(~PARENB);
    options.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
    options.c_cflag &= static_cast<tcflag_t>(~CSIZE);
    options.c_cflag |= CS8;
#ifdef CRTSCTS
    options.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
#endif
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    if (::tcsetattr(fd_, TCSANOW, &options) != 0) {
        error = errnoMessage("failed to apply MCU serial attributes");
        return false;
    }

    (void)::tcflush(fd_, TCIFLUSH);
    error.clear();
    return true;
#endif
}

bool McuSerialService::prepareConfiguredCommand(std::string& error)
{
#ifdef _WIN32
    error = "MCU serial command is only supported on Linux targets";
    return false;
#else
    commandFrame_.clear();
    commandBytesWritten_ = 0;
    commandSent_ = command_ == outdoor::mcu::McuCommand::None;
    if (command_ == outdoor::mcu::McuCommand::None) {
        error.clear();
        return true;
    }

    if (command_ == outdoor::mcu::McuCommand::Ping) {
        commandFrame_ = outdoor::mcu::buildPingCommandFrame(1U, outdoor::mcu::kDefaultPingNonce);
    } else {
        error = "unsupported MCU serial command";
        return false;
    }

    error.clear();
    return true;
#endif
}

outdoor::runtime::ServicePollResult McuSerialService::pollConfiguredCommand()
{
#ifdef _WIN32
    return outdoor::runtime::ServicePollResult::Failed;
#else
    if (commandSent_) {
        return outdoor::runtime::ServicePollResult::Completed;
    }

    const ssize_t writeSize = ::write(fd_,
                                      commandFrame_.data() + commandBytesWritten_,
                                      commandFrame_.size() - commandBytesWritten_);
    if (writeSize < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return outdoor::runtime::ServicePollResult::Running;
        }
        status_.lastError = errnoMessage("failed to write MCU serial command to " + devicePath_);
        outdoor::log::Logger::error(status_.lastError);
        return outdoor::runtime::ServicePollResult::Failed;
    }
    if (writeSize == 0) {
        return outdoor::runtime::ServicePollResult::Running;
    }
    commandBytesWritten_ += static_cast<std::size_t>(writeSize);

    if (commandBytesWritten_ < commandFrame_.size()) {
        return outdoor::runtime::ServicePollResult::Running;
    }

    commandSent_ = true;

    outdoor::log::Logger::info(std::string("MCU serial command sent: ") +
                               outdoor::mcu::mcuCommandToString(command_));
    return outdoor::runtime::ServicePollResult::Completed;
#endif
}

void McuSerialService::closePort()
{
#ifndef _WIN32
    if (fd_ >= 0) {
        (void)::close(fd_);
        fd_ = -1;
    }
#endif
}

} // namespace outdoor::services
