#include "log/logger.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace outdoor::log {

namespace {

LogLevel minimumLevel = LogLevel::Info;
std::ofstream fileOutput;
std::string activeFileOutputPath;
std::uintmax_t activeFileOutputSize = 0U;
std::uintmax_t maximumFileOutputSize = 0U;
std::size_t fileOutputBackupCount = 0U;
LoggerStatus loggerStatus;

void resetLoggerStatus()
{
    loggerStatus = {};
}

void recordRotationFailure(const std::string& error)
{
    ++loggerStatus.rotationFailureCount;
    loggerStatus.lastError = "log rotation failed: " + error;
}

void recordWriteFailure(const std::string& error)
{
    ++loggerStatus.writeFailureCount;
    loggerStatus.lastError = error;
}

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

std::filesystem::path backupPath(const std::filesystem::path& activePath, std::size_t index)
{
    return std::filesystem::path(activePath.string() + "." + std::to_string(index));
}

bool reopenActiveFile(std::ios::openmode mode, std::string& error)
{
    namespace fs = std::filesystem;

    fileOutput.clear();
    fileOutput.open(activeFileOutputPath, mode);
    if (!fileOutput.is_open()) {
        error = "failed to reopen log file: " + activeFileOutputPath;
        return false;
    }

    try {
        activeFileOutputSize = fs::exists(activeFileOutputPath)
            ? fs::file_size(activeFileOutputPath)
            : 0U;
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        return false;
    }
    return true;
}

bool rotateFileIfNeeded(std::size_t incomingBytes, std::string& error)
{
    namespace fs = std::filesystem;

    if (!fileOutput.is_open() || maximumFileOutputSize == 0U
        || activeFileOutputSize + incomingBytes <= maximumFileOutputSize) {
        error.clear();
        return true;
    }

    fileOutput.close();
    try {
        const fs::path activePath(activeFileOutputPath);
        if (fileOutputBackupCount > 0U) {
            fs::remove(backupPath(activePath, fileOutputBackupCount));
            for (std::size_t index = fileOutputBackupCount; index > 1U; --index) {
                const fs::path source = backupPath(activePath, index - 1U);
                const fs::path destination = backupPath(activePath, index);
                if (fs::exists(source)) {
                    fs::remove(destination);
                    fs::rename(source, destination);
                }
            }
            const fs::path firstBackup = backupPath(activePath, 1U);
            fs::remove(firstBackup);
            if (fs::exists(activePath)) {
                fs::rename(activePath, firstBackup);
            }
        } else {
            fs::remove(activePath);
        }
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        std::string reopenError;
        if (!reopenActiveFile(std::ios::app, reopenError)) {
            error += "; additionally " + reopenError;
        }
        return false;
    }

    return reopenActiveFile(std::ios::trunc, error);
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

bool Logger::setFileOutput(const std::string& filePath, std::string& error)
{
    return setFileOutput(filePath, 0U, 0U, error);
}

bool Logger::setFileOutput(const std::string& filePath,
                           std::uintmax_t maxBytes,
                           std::size_t backupCount,
                           std::string& error)
{
    namespace fs = std::filesystem;

    fileOutput.close();
    fileOutput.clear();
    activeFileOutputPath.clear();
    activeFileOutputSize = 0U;
    maximumFileOutputSize = 0U;
    fileOutputBackupCount = 0U;
    resetLoggerStatus();

    try {
        const fs::path path(filePath);
        const fs::path parent = path.parent_path();
        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        fileOutput.open(path, std::ios::app);
        if (!fileOutput.is_open()) {
            error = "failed to open log file: " + path.string();
            activeFileOutputPath.clear();
            return false;
        }
        activeFileOutputPath = path.string();
        activeFileOutputSize = fs::file_size(path);
        maximumFileOutputSize = maxBytes;
        fileOutputBackupCount = backupCount;
        loggerStatus.fileOutputEnabled = true;
    } catch (const fs::filesystem_error& exception) {
        fileOutput.close();
        fileOutput.clear();
        error = exception.what();
        activeFileOutputPath.clear();
        return false;
    }

    error.clear();
    return true;
}

void Logger::clearFileOutput()
{
    fileOutput.close();
    fileOutput.clear();
    activeFileOutputPath.clear();
    activeFileOutputSize = 0U;
    maximumFileOutputSize = 0U;
    fileOutputBackupCount = 0U;
    loggerStatus.fileOutputEnabled = false;
}

std::string Logger::fileOutputPath()
{
    return activeFileOutputPath;
}

LoggerStatus Logger::status()
{
    return loggerStatus;
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

    std::ostringstream line;
    line << "[" << currentTimestamp() << "] "
         << "[" << logLevelToString(level) << "] "
         << message << '\n';

    auto& output = (level == LogLevel::Error) ? std::cerr : std::cout;
    output << line.str();
    const std::string formattedLine = line.str();
    if (fileOutput.is_open()) {
        std::string rotationError;
        if (!rotateFileIfNeeded(formattedLine.size(), rotationError)) {
            recordRotationFailure(rotationError);
            std::cerr << "[LOGGER] " << loggerStatus.lastError << '\n';
        }
        if (fileOutput.is_open()) {
            fileOutput << formattedLine;
            fileOutput.flush();
            if (fileOutput) {
                activeFileOutputSize += formattedLine.size();
            } else {
                recordWriteFailure("failed to write or flush log file: " + activeFileOutputPath);
                std::cerr << "[LOGGER] " << loggerStatus.lastError << '\n';
            }
        }
    }
}

} // namespace outdoor::log
