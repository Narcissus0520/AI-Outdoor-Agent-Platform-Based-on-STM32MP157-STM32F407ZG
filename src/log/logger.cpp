#include "log/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace outdoor::log {

namespace {

const char* levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }

    return "UNKNOWN";
}

std::string currentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

} // namespace

void Logger::info(const std::string& message)
{
    write(LogLevel::Info, message);
}

void Logger::warn(const std::string& message)
{
    write(LogLevel::Warn, message);
}

void Logger::error(const std::string& message)
{
    write(LogLevel::Error, message);
}

void Logger::write(LogLevel level, const std::string& message)
{
    auto& output = (level == LogLevel::Error) ? std::cerr : std::cout;
    output << "[" << currentTimestamp() << "] "
           << "[" << levelToString(level) << "] "
           << message << '\n';
}

} // namespace outdoor::log
