#include "log/logger.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

std::string readText(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    namespace fs = std::filesystem;

    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path output = fs::temp_directory_path()
        / ("outdoor_logger_test_" + std::to_string(unique));
    fs::create_directories(output);

    const fs::path rotatingLog = output / "rotating.log";
    std::string error;
    expect(outdoor::log::Logger::setFileOutput(rotatingLog.string(), 180U, 2U, error),
           "rotating logger should open: " + error);
    outdoor::log::Logger::setMinimumLevel(outdoor::log::LogLevel::Info);

    const std::string padding(100U, 'x');
    for (int index = 0; index < 8; ++index) {
        outdoor::log::Logger::info("rotation-message-" + std::to_string(index) + " " + padding);
    }
    const auto rotatingStatus = outdoor::log::Logger::status();
    expect(rotatingStatus.fileOutputEnabled, "rotating logger should report file output enabled");
    expect(rotatingStatus.rotationFailureCount == 0U,
           "successful rotation should not increment the rotation failure count");
    expect(rotatingStatus.writeFailureCount == 0U,
           "successful writes should not increment the write failure count");
    expect(rotatingStatus.lastError.empty(), "successful rotation should not report a logger error");
    outdoor::log::Logger::clearFileOutput();

    expect(fs::exists(rotatingLog), "active log should exist after rotation");
    expect(fs::exists(rotatingLog.string() + ".1"), "first log backup should exist");
    expect(fs::exists(rotatingLog.string() + ".2"), "second log backup should exist");
    expect(!fs::exists(rotatingLog.string() + ".3"), "rotation should enforce the backup limit");
    expect(readText(rotatingLog).find("rotation-message-7") != std::string::npos,
           "active log should contain the newest message");
    expect(readText(rotatingLog.string() + ".1").find("rotation-message-6") != std::string::npos,
           "first backup should contain the previous message");

    const fs::path unlimitedLog = output / "unlimited.log";
    expect(outdoor::log::Logger::setFileOutput(unlimitedLog.string(), error),
           "unlimited logger should open: " + error);
    for (int index = 0; index < 4; ++index) {
        outdoor::log::Logger::info("unlimited-message-" + std::to_string(index));
    }
    outdoor::log::Logger::clearFileOutput();
    expect(!fs::exists(unlimitedLog.string() + ".1"), "legacy logger overload should keep rotation disabled");

    const fs::path recoveryLog = output / "recovery.log";
    expect(outdoor::log::Logger::setFileOutput(recoveryLog.string(), 1U, 1U, error),
           "recovery logger should open: " + error);
    const fs::path blockedBackup = recoveryLog.string() + ".1";
    fs::create_directories(blockedBackup);
    {
        std::ofstream blocker(blockedBackup / "keep.txt");
        blocker << "prevent directory removal";
    }

    outdoor::log::Logger::info("rotation-failure-should-remain-writable");
    const auto recoveryStatus = outdoor::log::Logger::status();
    expect(recoveryStatus.fileOutputEnabled,
           "logger should keep file output enabled after a recoverable rotation failure");
    expect(recoveryStatus.rotationFailureCount == 1U,
           "failed rotation should increment the rotation failure count once");
    expect(recoveryStatus.writeFailureCount == 0U,
           "reopened active log should remain writable after rotation failure");
    expect(recoveryStatus.lastError.find("log rotation failed:") != std::string::npos,
           "failed rotation should preserve a structured last error");
    outdoor::log::Logger::clearFileOutput();
    expect(readText(recoveryLog).find("rotation-failure-should-remain-writable") != std::string::npos,
           "recoverable rotation failure should still append the triggering message");

    fs::remove_all(output);
    std::cout << "logger_tests passed\n";
    return 0;
}
