#include "services/gnss_serial_service.h"

#include "gnss/gnss_status.h"
#include "log/logger.h"

#include <array>
#include <chrono>
#include <string>
#include <utility>

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
    default:
        return false;
    }
}

std::string errnoMessage(const std::string& prefix)
{
    return prefix + ": " + std::strerror(errno);
}
#endif

std::string sentenceType(const std::string& line)
{
    if (line.size() < 6 || line[0] != '$') {
        return "unknown";
    }
    return line.substr(3, 3);
}

} // namespace

GnssSerialService::GnssSerialService(std::string devicePath,
                                     std::uint32_t baudRate,
                                     std::uint32_t captureSeconds,
                                     outdoor::gnss::GnssStatus& status,
                                     std::function<bool(std::string&)> statusUpdatedCallback)
    : devicePath_(std::move(devicePath)),
      baudRate_(baudRate),
      captureSeconds_(captureSeconds),
      status_(status),
      statusUpdatedCallback_(std::move(statusUpdatedCallback))
{
    status_.enabled = true;
    status_.source = "uart5";
}

GnssSerialService::~GnssSerialService()
{
    closePort();
}

const char* GnssSerialService::name() const
{
    return "gnss_serial_service";
}

bool GnssSerialService::start()
{
#ifdef _WIN32
    status_.lastError = "GNSS serial input is only supported on Linux targets";
    outdoor::log::Logger::error(status_.lastError);
    return false;
#else
    fd_ = ::open(devicePath_.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        status_.lastError = errnoMessage("failed to open GNSS serial device " + devicePath_);
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    std::string error;
    if (!configurePort(error)) {
        status_.lastError = error;
        outdoor::log::Logger::error(error);
        closePort();
        return false;
    }

    deadlineEnabled_ = captureSeconds_ > 0U;
    if (deadlineEnabled_) {
        deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(captureSeconds_);
    }
    lineBuffer_.clear();
    outdoor::log::Logger::info("GNSS serial input opened: " + devicePath_);
    return true;
#endif
}

outdoor::runtime::ServicePollResult GnssSerialService::poll()
{
#ifdef _WIN32
    return outdoor::runtime::ServicePollResult::Failed;
#else
    if (fd_ < 0) {
        status_.lastError = "GNSS serial device is not open";
        outdoor::log::Logger::error(status_.lastError);
        return outdoor::runtime::ServicePollResult::Failed;
    }

    if (deadlineEnabled_ && std::chrono::steady_clock::now() >= deadline_) {
        if (!lineBuffer_.empty()) {
            if (!consumeLine(lineBuffer_)) {
                return outdoor::runtime::ServicePollResult::Failed;
            }
            lineBuffer_.clear();
        }
        if (!status_.seen) {
            status_.lastError = "GNSS serial capture completed without valid NMEA sentences";
            outdoor::log::Logger::warn(status_.lastError);
            return outdoor::runtime::ServicePollResult::Failed;
        }
        return outdoor::runtime::ServicePollResult::Completed;
    }

    std::array<char, 256U> buffer {};
    const ssize_t readSize = ::read(fd_, buffer.data(), buffer.size());
    if (readSize < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return outdoor::runtime::ServicePollResult::Running;
        }
        status_.lastError = errnoMessage("failed to read GNSS serial device " + devicePath_);
        outdoor::log::Logger::error(status_.lastError);
        return outdoor::runtime::ServicePollResult::Failed;
    }

    if (readSize == 0) {
        return outdoor::runtime::ServicePollResult::Running;
    }

    for (ssize_t index = 0; index < readSize; ++index) {
        const char ch = buffer[static_cast<std::size_t>(index)];
        if (ch == '\n') {
            if (!consumeLine(lineBuffer_)) {
                return outdoor::runtime::ServicePollResult::Failed;
            }
            lineBuffer_.clear();
        } else if (ch != '\r') {
            lineBuffer_.push_back(ch);
            if (lineBuffer_.size() > 160U) {
                status_.lastError = "GNSS NMEA line exceeded maximum length";
                ++status_.skippedSentenceCount;
                lineBuffer_.clear();
            }
        }
    }

    return outdoor::runtime::ServicePollResult::Running;
#endif
}

void GnssSerialService::stop()
{
    closePort();
}

bool GnssSerialService::configurePort(std::string& error)
{
#ifdef _WIN32
    error = "GNSS serial input is only supported on Linux targets";
    return false;
#else
    speed_t speed {};
    if (!baudToSpeed(baudRate_, speed)) {
        error = "unsupported GNSS serial baud rate: " + std::to_string(baudRate_);
        return false;
    }

    termios options {};
    if (::tcgetattr(fd_, &options) != 0) {
        error = errnoMessage("failed to read GNSS serial attributes");
        return false;
    }

    ::cfmakeraw(&options);
    if (::cfsetispeed(&options, speed) != 0 || ::cfsetospeed(&options, speed) != 0) {
        error = errnoMessage("failed to set GNSS serial baud rate");
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
        error = errnoMessage("failed to apply GNSS serial attributes");
        return false;
    }

    (void)::tcflush(fd_, TCIFLUSH);
    error.clear();
    return true;
#endif
}

void GnssSerialService::closePort()
{
#ifndef _WIN32
    if (fd_ >= 0) {
        (void)::close(fd_);
        fd_ = -1;
    }
#endif
}

bool GnssSerialService::consumeLine(const std::string& line)
{
    if (line.empty()) {
        return true;
    }

    status_.lastSentenceType = sentenceType(line);
    outdoor::log::Logger::info("GNSS UART5 NMEA: " + line);

    std::string parseError;
    if (parser_.parseLine(line, status_.fix, parseError)) {
        status_.seen = true;
        ++status_.validSentenceCount;
        status_.lastError.clear();
        outdoor::log::Logger::info(outdoor::gnss::formatGnssFix(status_.fix));
        if (statusUpdatedCallback_) {
            std::string callbackError;
            if (!statusUpdatedCallback_(callbackError)) {
                status_.lastError = "GNSS status update callback failed: " + callbackError;
                outdoor::log::Logger::error(status_.lastError);
                return false;
            }
        }
        return true;
    }

    if (!parseError.empty()) {
        ++status_.skippedSentenceCount;
        status_.lastError = parseError;
        outdoor::log::Logger::debug("GNSS UART5 NMEA parse skipped: " + parseError);
    }
    return true;
}

} // namespace outdoor::services
