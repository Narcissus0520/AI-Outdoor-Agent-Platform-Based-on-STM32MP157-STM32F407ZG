#include "log/logger.h"

#include <chrono>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace outdoor::log {

namespace {

LogLevel minimumLevel = LogLevel::Info;

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
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

bool parseLogLevel(const std::string& value, LogLevel& level)
{
    const std::string normalized = toLower(value);
    if (normalized == "debug") {
        level = LogLevel::Debug;
        return true;
    }
    if (normalized == "info") {
        level = LogLevel::Info;
        return true;
    }
    if (normalized == "warn" || normalized == "warning") {
        level = LogLevel::Warn;
        return true;
    }
    if (normalized == "error") {
        level = LogLevel::Error;
        return true;
    }

    return false;
}

const char* logLevelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }

    return "UNKNOWN";
}

void Logger::setMinimumLevel(LogLevel level)
{
    minimumLevel = level;
}

void Logger::debug(const std::string& message)
{
    write(LogLevel::Debug, message);
}

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
    if (level < minimumLevel) {
        return;
    }

    auto& output = (level == LogLevel::Error) ? std::cerr : std::cout;
    output << "[" << currentTimestamp() << "] "
           << "[" << logLevelToString(level) << "] "
           << message << '\n';
}

} // namespace outdoor::log
