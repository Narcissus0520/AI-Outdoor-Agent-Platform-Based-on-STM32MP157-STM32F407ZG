#pragma once

#include <string>

namespace outdoor::log {

enum class LogLevel {
    Info,
    Warn,
    Error,
};

class Logger {
public:
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static void write(LogLevel level, const std::string& message);
};

} // namespace outdoor::log
