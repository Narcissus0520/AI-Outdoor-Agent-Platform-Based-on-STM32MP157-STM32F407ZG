#include "config/config_loader.h"
#include "ipc/status_publisher.h"
#include "log/logger.h"
#include "mcu/mcu_status.h"
#include "runtime/runtime_manager.h"
#include "services/gnss_replay_service.h"
#include "services/icm20608_service.h"
#include "services/mcu_mock_service.h"
#include "sensors/board_imu_status.h"
#include "sensors/icm20608_char_reader.h"
#include "sensors/icm20608_iio_reader.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

constexpr const char* kDefaultNmeaFile = "data/nmea_sample.txt";
constexpr const char* kDefaultMcuMockFile = "data/mcu_mock_frames.txt";
constexpr const char* kDefaultConfigFile = "config/runtime.conf";
constexpr const char* kDefaultStatusOutputFile = "runtime/runtime_status.json";

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [nmea_file]\n"
              << "       " << programName
              << " [--config path] [--input path] [--mcu-mock-input path] [--status-output path]"
              << " [--board-imu] [--board-imu-source char_device|iio|auto]"
              << " [--board-imu-device-path path] [--board-imu-iio-path path] [--board-imu-samples count]"
              << " [--log-level debug|info|warn|error]\n";
}

void publishStatus(const outdoor::ipc::StatusPublisher& publisher,
                   const outdoor::runtime::RuntimeStatus& status)
{
    std::string error;
    if (!publisher.publish(status, error)) {
        outdoor::log::Logger::warn("Failed to publish runtime status: " + error);
    }
}

bool parseSizeArgument(const std::string& value, std::size_t& parsed)
{
    try {
        std::size_t consumed = 0;
        const auto result = std::stoull(value, &consumed);
        if (consumed != value.size()) {
            return false;
        }
        parsed = static_cast<std::size_t>(result);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    outdoor::config::AppConfig config;
    config.nmeaInputPath = kDefaultNmeaFile;
    config.mcuMockInputPath = kDefaultMcuMockFile;
    config.statusOutputPath = kDefaultStatusOutputFile;

    std::string configPath = kDefaultConfigFile;
    std::string cliInputPath;
    std::string cliMcuMockInputPath;
    std::string cliLogLevel;
    std::string cliStatusOutputPath;
    bool cliEnableBoardImu = false;
    bool cliDisableBoardImu = false;
    std::string cliBoardImuSource;
    std::string cliBoardImuDevicePath;
    std::string cliBoardImuIioPath;
    std::string cliBoardImuSampleCount;

    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--config") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--config requires a file path");
                return 1;
            }
            configPath = argv[++index];
        } else if (arg == "--input") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--input requires a file path");
                return 1;
            }
            cliInputPath = argv[++index];
        } else if (arg == "--mcu-mock-input") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-mock-input requires a file path");
                return 1;
            }
            cliMcuMockInputPath = argv[++index];
        } else if (arg == "--status-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--status-output requires a file path");
                return 1;
            }
            cliStatusOutputPath = argv[++index];
        } else if (arg == "--board-imu") {
            cliEnableBoardImu = true;
            cliDisableBoardImu = false;
        } else if (arg == "--no-board-imu") {
            cliDisableBoardImu = true;
            cliEnableBoardImu = false;
        } else if (arg == "--board-imu-source") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--board-imu-source requires char_device, iio, or auto");
                return 1;
            }
            cliBoardImuSource = argv[++index];
        } else if (arg == "--board-imu-device-path") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--board-imu-device-path requires a file path");
                return 1;
            }
            cliBoardImuDevicePath = argv[++index];
        } else if (arg == "--board-imu-iio-path") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--board-imu-iio-path requires a directory path");
                return 1;
            }
            cliBoardImuIioPath = argv[++index];
        } else if (arg == "--board-imu-samples") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--board-imu-samples requires a count");
                return 1;
            }
            cliBoardImuSampleCount = argv[++index];
        } else if (arg == "--log-level") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--log-level requires a value");
                return 1;
            }
            cliLogLevel = argv[++index];
        } else if (arg.rfind("--", 0) == 0) {
            outdoor::log::Logger::error("Unknown argument: " + arg);
            printUsage(argv[0]);
            return 1;
        } else {
            cliInputPath = arg;
        }
    }

    outdoor::config::ConfigLoader loader;
    std::string configError;
    if (!loader.load(configPath, config, configError)) {
        outdoor::log::Logger::warn(configError + "; using built-in defaults and CLI overrides");
    }

    if (!cliInputPath.empty()) {
        config.nmeaInputPath = cliInputPath;
    }

    if (!cliMcuMockInputPath.empty()) {
        config.mcuMockInputPath = cliMcuMockInputPath;
    }

    if (!cliStatusOutputPath.empty()) {
        config.statusOutputPath = cliStatusOutputPath;
    }

    if (cliEnableBoardImu) {
        config.boardImuEnabled = true;
    }

    if (cliDisableBoardImu) {
        config.boardImuEnabled = false;
    }

    if (!cliBoardImuSource.empty()) {
        if (cliBoardImuSource != "char_device" && cliBoardImuSource != "iio" && cliBoardImuSource != "auto") {
            outdoor::log::Logger::error("Unsupported --board-imu-source value: " + cliBoardImuSource);
            return 1;
        }
        config.boardImuSource = cliBoardImuSource;
    }

    if (!cliBoardImuDevicePath.empty()) {
        config.boardImuDevicePath = cliBoardImuDevicePath;
    }

    if (!cliBoardImuIioPath.empty()) {
        config.boardImuIioPath = cliBoardImuIioPath;
    }

    if (!cliBoardImuSampleCount.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliBoardImuSampleCount, parsed)) {
            outdoor::log::Logger::error("Unsupported --board-imu-samples value: " + cliBoardImuSampleCount);
            return 1;
        }
        config.boardImuSampleCount = parsed;
    }

    if (!cliLogLevel.empty()) {
        outdoor::log::LogLevel parsedLevel {};
        if (!outdoor::log::parseLogLevel(cliLogLevel, parsedLevel)) {
            outdoor::log::Logger::error("Unsupported --log-level value: " + cliLogLevel);
            return 1;
        }
        config.logLevel = parsedLevel;
    }

    outdoor::log::Logger::setMinimumLevel(config.logLevel);

    outdoor::log::Logger::info("Outdoor Core Runtime starting");
    outdoor::log::Logger::info("Config file: " + configPath);
    outdoor::log::Logger::info(std::string("Log level: ") + outdoor::log::logLevelToString(config.logLevel));
    outdoor::log::Logger::info("Input source: " + config.nmeaInputPath);
    outdoor::log::Logger::info("MCU mock input source: " + config.mcuMockInputPath);
    outdoor::log::Logger::info(std::string("Board IMU enabled: ") + (config.boardImuEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Board IMU source: " + config.boardImuSource);
    outdoor::log::Logger::info("Board IMU character device path: " + config.boardImuDevicePath);
    outdoor::log::Logger::info("Board IMU IIO path: " + config.boardImuIioPath);
    outdoor::log::Logger::info("Runtime status output: " + config.statusOutputPath);

    outdoor::runtime::RuntimeManager runtime;
    outdoor::mcu::McuStatus mcuStatus;
    outdoor::sensors::BoardImuStatus boardImuStatus;
    boardImuStatus.enabled = config.boardImuEnabled;
    boardImuStatus.source = config.boardImuSource == "iio" ? "icm20608_iio" : "icm20608_char";
    boardImuStatus.devicePath = config.boardImuSource == "iio" ? config.boardImuIioPath : config.boardImuDevicePath;
    runtime.addService(std::make_unique<outdoor::services::GnssReplayService>(config.nmeaInputPath));
    runtime.addService(std::make_unique<outdoor::services::McuMockService>(config.mcuMockInputPath, mcuStatus));
    if (config.boardImuEnabled) {
        runtime.addService(std::make_unique<outdoor::services::Icm20608Service>(
            config.boardImuSource,
            outdoor::sensors::Icm20608CharReader(config.boardImuDevicePath),
            outdoor::sensors::Icm20608IioReader(config.boardImuIioPath),
            boardImuStatus,
            config.boardImuSampleCount,
            config.boardImuSampleIntervalMs));
    }

    outdoor::ipc::StatusPublisher statusPublisher(config.statusOutputPath);
    runtime.setMcuStatus(mcuStatus);
    runtime.setBoardImuStatus(boardImuStatus);
    publishStatus(statusPublisher, runtime.status());

    if (!runtime.start()) {
        runtime.setMcuStatus(mcuStatus);
        runtime.setBoardImuStatus(boardImuStatus);
        publishStatus(statusPublisher, runtime.status());
        outdoor::log::Logger::error("Outdoor Core Runtime failed to start");
        return 1;
    }
    runtime.setMcuStatus(mcuStatus);
    runtime.setBoardImuStatus(boardImuStatus);
    publishStatus(statusPublisher, runtime.status());

    if (!runtime.run()) {
        runtime.stop();
        runtime.setMcuStatus(mcuStatus);
        runtime.setBoardImuStatus(boardImuStatus);
        publishStatus(statusPublisher, runtime.status());
        outdoor::log::Logger::error("Outdoor Core Runtime failed while running");
        return 1;
    }

    runtime.stop();
    runtime.setMcuStatus(mcuStatus);
    runtime.setBoardImuStatus(boardImuStatus);
    publishStatus(statusPublisher, runtime.status());
    outdoor::log::Logger::info("Outdoor Core Runtime stopped");
    return 0;
}
