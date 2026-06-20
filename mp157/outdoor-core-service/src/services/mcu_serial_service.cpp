#include "services/mcu_serial_service.h"

#include "log/logger.h"

#include <array>
#include <chrono>
#include <string>
#include <thread>
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
                                   outdoor::mcu::McuStatus& status)
    : devicePath_(std::move(devicePath)),
      baudRate_(baudRate),
      captureSeconds_(captureSeconds),
      status_(status)
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
    fd_ = ::open(devicePath_.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
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
    outdoor::log::Logger::info("MCU serial input opened: " + devicePath_);
    return true;
#endif
}

bool McuSerialService::run()
{
#ifdef _WIN32
    return false;
#else
    if (fd_ < 0) {
        status_.lastError = "MCU serial device is not open";
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(captureSeconds_);
    std::array<std::uint8_t, 256U> buffer {};
    while (std::chrono::steady_clock::now() < deadline) {
        const ssize_t readSize = ::read(fd_, buffer.data(), buffer.size());
        if (readSize < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            status_.lastError = errnoMessage("failed to read MCU serial device " + devicePath_);
            outdoor::log::Logger::error(status_.lastError);
            return false;
        }

        if (readSize == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        std::vector<std::vector<std::uint8_t>> rawFrames;
        std::string error;
        if (!streamDecoder_.feed(buffer.data(), static_cast<std::size_t>(readSize), rawFrames, error)) {
            status_.lastError = error;
            outdoor::log::Logger::warn(error);
            continue;
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

            outdoor::log::Logger::info(outdoor::mcu::formatMcuStatus(status_));
        }
    }

    if (!status_.heartbeatSeen && !status_.imuSeen) {
        status_.lastError = "MCU serial capture completed without heartbeat or IMU frames";
        outdoor::log::Logger::warn(status_.lastError);
        return false;
    }

    return true;
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
