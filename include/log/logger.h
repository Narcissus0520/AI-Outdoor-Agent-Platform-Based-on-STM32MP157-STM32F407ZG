#pragma once

#include <string>

namespace outdoor::log {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

bool parseLogLevel(const std::string& value, LogLevel& level);
const char* logLevelToString(LogLevel level);

class Logger {
public:
    static void setMinimumLevel(LogLevel level);

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static void write(LogLevel level, const std::string& message);
};

} // namespace outdoor::log
