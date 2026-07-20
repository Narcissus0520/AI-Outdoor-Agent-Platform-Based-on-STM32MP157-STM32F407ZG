#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace outdoor::log {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

struct LoggerStatus {
    bool fileOutputEnabled = false;
    std::uint64_t rotationFailureCount = 0U;
    std::uint64_t writeFailureCount = 0U;
    std::string lastError;
};

bool parseLogLevel(const std::string& value, LogLevel& level);
const char* logLevelToString(LogLevel level);

class Logger {
public:
    static void setMinimumLevel(LogLevel level);
    static bool setFileOutput(const std::string& filePath, std::string& error);
    static bool setFileOutput(const std::string& filePath,
                              std::uintmax_t maxBytes,
                              std::size_t backupCount,
                              std::string& error);
    static void clearFileOutput();
    static std::string fileOutputPath();
    static LoggerStatus status();

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static void write(LogLevel level, const std::string& message);
};

} // namespace outdoor::log
