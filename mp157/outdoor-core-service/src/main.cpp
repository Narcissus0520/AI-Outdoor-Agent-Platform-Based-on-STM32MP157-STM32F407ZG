#include "config/config_loader.h"
#include "gnss/gnss_status.h"
#include "ipc/status_publisher.h"
#include "log/logger.h"
#include "mcu/mcu_command.h"
#include "mcu/mcu_status.h"
#include "runtime/runtime_manager.h"
#include "services/dashboard_service.h"
#include "services/gnss_replay_service.h"
#include "services/gnss_serial_service.h"
#include "services/icm20608_service.h"
#include "services/mcu_mock_service.h"
#include "services/mcu_serial_service.h"
#include "sensors/board_imu_status.h"
#include "sensors/icm20608_char_reader.h"
#include "sensors/icm20608_iio_reader.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
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
              << " [--config path] [--input path] [--gnss-input-mode file|serial]"
              << " [--gnss-serial-device path] [--gnss-serial-baud baud]"
              << " [--gnss-serial-capture-seconds seconds]"
              << " [--mcu-input-mode mock_file|serial]"
              << " [--mcu-mock-input path] [--mcu-serial-device path]"
              << " [--mcu-serial-baud baud] [--mcu-serial-capture-seconds seconds]"
              << " [--mcu-command none|ping]"
              << " [--status-output path]"
              << " [--storage] [--no-storage] [--storage-root path]"
              << " [--storage-status-output path] [--storage-dashboard-output path]"
              << " [--storage-log-file path]"
              << " [--board-imu] [--board-imu-source char_device|iio|auto]"
              << " [--board-imu-device-path path] [--board-imu-iio-path path] [--board-imu-samples count]"
              << " [--dashboard-output path] [--dashboard-output-mode text|framebuffer|both]"
              << " [--dashboard-framebuffer-device path]"
              << " [--dashboard-refresh-count count] [--dashboard-refresh-interval-ms ms]"
              << " [--no-dashboard]"
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

std::string resolveStoragePath(const std::string& rootPath, const std::string& configuredPath)
{
    namespace fs = std::filesystem;

    const fs::path path(configuredPath);
    if (path.is_absolute()) {
        return path.lexically_normal().string();
    }

    return (fs::path(rootPath) / path).lexically_normal().string();
}

bool prepareStorage(outdoor::config::AppConfig& config,
                    bool statusOutputOverridden,
                    bool dashboardOutputOverridden,
                    std::string& error)
{
    namespace fs = std::filesystem;

    if (!config.storageEnabled) {
        outdoor::log::Logger::clearFileOutput();
        error.clear();
        return true;
    }

    if (config.storageRootPath.empty()) {
        error = "storage_root_path must not be empty when storage is enabled";
        return false;
    }

    try {
        const fs::path root(config.storageRootPath);
        fs::create_directories(root);
        fs::create_directories(root / "logs");
        fs::create_directories(root / "status");
        fs::create_directories(root / "dashboard");
        fs::create_directories(root / "data");
        fs::create_directories(root / "captures");

        if (!statusOutputOverridden) {
            config.statusOutputPath = resolveStoragePath(config.storageRootPath, config.storageStatusOutputPath);
        }
        if (!dashboardOutputOverridden) {
            config.dashboardOutputPath = resolveStoragePath(config.storageRootPath, config.storageDashboardOutputPath);
        }

        const std::string logFilePath = resolveStoragePath(config.storageRootPath, config.storageLogFilePath);
        if (!outdoor::log::Logger::setFileOutput(logFilePath, error)) {
            return false;
        }
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        return false;
    }

    error.clear();
    return true;
}

outdoor::runtime::RuntimeStatus buildStatus(const outdoor::runtime::RuntimeManager& runtime,
                                            const outdoor::config::AppConfig& config,
                                            const std::string& storageLastError)
{
    outdoor::runtime::RuntimeStatus status = runtime.status();
    status.storageEnabled = config.storageEnabled;
    status.storageRootPath = config.storageRootPath;
    status.storageStatusOutputPath = config.statusOutputPath;
    status.storageDashboardOutputPath = config.dashboardOutputPath;
    status.storageLogFilePath = outdoor::log::Logger::fileOutputPath();
    status.storageLastError = storageLastError;
    return status;
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
    std::string cliGnssInputMode;
    std::string cliGnssSerialDevice;
    std::string cliGnssSerialBaud;
    std::string cliGnssSerialCaptureSeconds;
    std::string cliMcuInputMode;
    std::string cliMcuMockInputPath;
    std::string cliMcuSerialDevice;
    std::string cliMcuSerialBaud;
    std::string cliMcuSerialCaptureSeconds;
    std::string cliMcuCommand;
    std::string cliLogLevel;
    std::string cliStatusOutputPath;
    bool cliEnableStorage = false;
    bool cliDisableStorage = false;
    std::string cliStorageRootPath;
    std::string cliStorageStatusOutputPath;
    std::string cliStorageDashboardOutputPath;
    std::string cliStorageLogFilePath;
    bool cliEnableBoardImu = false;
    bool cliDisableBoardImu = false;
    bool cliDisableDashboard = false;
    std::string cliDashboardOutputPath;
    std::string cliDashboardOutputMode;
    std::string cliDashboardFramebufferDevice;
    std::string cliDashboardRefreshCount;
    std::string cliDashboardRefreshIntervalMs;
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
        } else if (arg == "--gnss-input-mode") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--gnss-input-mode requires file or serial");
                return 1;
            }
            cliGnssInputMode = argv[++index];
        } else if (arg == "--gnss-serial-device") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--gnss-serial-device requires a device path");
                return 1;
            }
            cliGnssSerialDevice = argv[++index];
        } else if (arg == "--gnss-serial-baud") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--gnss-serial-baud requires a baud rate");
                return 1;
            }
            cliGnssSerialBaud = argv[++index];
        } else if (arg == "--gnss-serial-capture-seconds") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--gnss-serial-capture-seconds requires seconds");
                return 1;
            }
            cliGnssSerialCaptureSeconds = argv[++index];
        } else if (arg == "--mcu-mock-input") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-mock-input requires a file path");
                return 1;
            }
            cliMcuMockInputPath = argv[++index];
        } else if (arg == "--mcu-input-mode") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-input-mode requires mock_file or serial");
                return 1;
            }
            cliMcuInputMode = argv[++index];
        } else if (arg == "--mcu-serial-device") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-serial-device requires a device path");
                return 1;
            }
            cliMcuSerialDevice = argv[++index];
        } else if (arg == "--mcu-serial-baud") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-serial-baud requires a baud rate");
                return 1;
            }
            cliMcuSerialBaud = argv[++index];
        } else if (arg == "--mcu-serial-capture-seconds") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-serial-capture-seconds requires seconds");
                return 1;
            }
            cliMcuSerialCaptureSeconds = argv[++index];
        } else if (arg == "--mcu-command") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--mcu-command requires none or ping");
                return 1;
            }
            cliMcuCommand = argv[++index];
        } else if (arg == "--status-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--status-output requires a file path");
                return 1;
            }
            cliStatusOutputPath = argv[++index];
        } else if (arg == "--storage") {
            cliEnableStorage = true;
            cliDisableStorage = false;
        } else if (arg == "--no-storage") {
            cliDisableStorage = true;
            cliEnableStorage = false;
        } else if (arg == "--storage-root") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--storage-root requires a directory path");
                return 1;
            }
            cliStorageRootPath = argv[++index];
            cliEnableStorage = true;
            cliDisableStorage = false;
        } else if (arg == "--storage-status-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--storage-status-output requires a file path");
                return 1;
            }
            cliStorageStatusOutputPath = argv[++index];
        } else if (arg == "--storage-dashboard-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--storage-dashboard-output requires a file path");
                return 1;
            }
            cliStorageDashboardOutputPath = argv[++index];
        } else if (arg == "--storage-log-file") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--storage-log-file requires a file path");
                return 1;
            }
            cliStorageLogFilePath = argv[++index];
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
        } else if (arg == "--dashboard-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--dashboard-output requires a file path");
                return 1;
            }
            cliDashboardOutputPath = argv[++index];
        } else if (arg == "--dashboard-output-mode") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--dashboard-output-mode requires text, framebuffer, or both");
                return 1;
            }
            cliDashboardOutputMode = argv[++index];
        } else if (arg == "--dashboard-framebuffer-device") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--dashboard-framebuffer-device requires a device path");
                return 1;
            }
            cliDashboardFramebufferDevice = argv[++index];
        } else if (arg == "--dashboard-refresh-count") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--dashboard-refresh-count requires a count");
                return 1;
            }
            cliDashboardRefreshCount = argv[++index];
        } else if (arg == "--dashboard-refresh-interval-ms") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--dashboard-refresh-interval-ms requires milliseconds");
                return 1;
            }
            cliDashboardRefreshIntervalMs = argv[++index];
        } else if (arg == "--no-dashboard") {
            cliDisableDashboard = true;
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

    if (!cliGnssInputMode.empty()) {
        if (cliGnssInputMode != "file" && cliGnssInputMode != "serial") {
            outdoor::log::Logger::error("Unsupported --gnss-input-mode value: " + cliGnssInputMode);
            return 1;
        }
        config.gnssInputMode = cliGnssInputMode;
    }

    if (!cliGnssSerialDevice.empty()) {
        config.gnssSerialDevice = cliGnssSerialDevice;
    }

    if (!cliGnssSerialBaud.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliGnssSerialBaud, parsed)) {
            outdoor::log::Logger::error("Unsupported --gnss-serial-baud value: " + cliGnssSerialBaud);
            return 1;
        }
        config.gnssSerialBaud = static_cast<std::uint32_t>(parsed);
    }

    if (!cliGnssSerialCaptureSeconds.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliGnssSerialCaptureSeconds, parsed)) {
            outdoor::log::Logger::error("Unsupported --gnss-serial-capture-seconds value: " + cliGnssSerialCaptureSeconds);
            return 1;
        }
        config.gnssSerialCaptureSeconds = static_cast<std::uint32_t>(parsed);
    }

    if (!cliMcuMockInputPath.empty()) {
        config.mcuMockInputPath = cliMcuMockInputPath;
    }

    if (!cliMcuInputMode.empty()) {
        if (cliMcuInputMode != "mock_file" && cliMcuInputMode != "serial") {
            outdoor::log::Logger::error("Unsupported --mcu-input-mode value: " + cliMcuInputMode);
            return 1;
        }
        config.mcuInputMode = cliMcuInputMode;
    }

    if (!cliMcuSerialDevice.empty()) {
        config.mcuSerialDevice = cliMcuSerialDevice;
    }

    if (!cliMcuSerialBaud.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliMcuSerialBaud, parsed)) {
            outdoor::log::Logger::error("Unsupported --mcu-serial-baud value: " + cliMcuSerialBaud);
            return 1;
        }
        config.mcuSerialBaud = static_cast<std::uint32_t>(parsed);
    }

    if (!cliMcuSerialCaptureSeconds.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliMcuSerialCaptureSeconds, parsed)) {
            outdoor::log::Logger::error("Unsupported --mcu-serial-capture-seconds value: " + cliMcuSerialCaptureSeconds);
            return 1;
        }
        config.mcuSerialCaptureSeconds = static_cast<std::uint32_t>(parsed);
    }

    if (!cliMcuCommand.empty()) {
        outdoor::mcu::McuCommand parsedCommand {};
        if (!outdoor::mcu::parseMcuCommand(cliMcuCommand, parsedCommand)) {
            outdoor::log::Logger::error("Unsupported --mcu-command value: " + cliMcuCommand);
            return 1;
        }
        config.mcuCommand = cliMcuCommand;
    }

    if (!cliStatusOutputPath.empty()) {
        config.statusOutputPath = cliStatusOutputPath;
    }

    if (cliEnableStorage) {
        config.storageEnabled = true;
    }

    if (cliDisableStorage) {
        config.storageEnabled = false;
    }

    if (!cliStorageRootPath.empty()) {
        config.storageRootPath = cliStorageRootPath;
    }

    if (!cliStorageStatusOutputPath.empty()) {
        config.storageStatusOutputPath = cliStorageStatusOutputPath;
    }

    if (!cliStorageDashboardOutputPath.empty()) {
        config.storageDashboardOutputPath = cliStorageDashboardOutputPath;
    }

    if (!cliStorageLogFilePath.empty()) {
        config.storageLogFilePath = cliStorageLogFilePath;
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

    if (!cliDashboardOutputPath.empty()) {
        config.dashboardOutputPath = cliDashboardOutputPath;
    }

    if (!cliDashboardOutputMode.empty()) {
        if (cliDashboardOutputMode != "text" && cliDashboardOutputMode != "framebuffer" && cliDashboardOutputMode != "both") {
            outdoor::log::Logger::error("Unsupported --dashboard-output-mode value: " + cliDashboardOutputMode);
            return 1;
        }
        config.dashboardOutputMode = cliDashboardOutputMode;
    }

    if (!cliDashboardFramebufferDevice.empty()) {
        config.dashboardFramebufferDevice = cliDashboardFramebufferDevice;
    }

    if (!cliDashboardRefreshCount.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliDashboardRefreshCount, parsed)) {
            outdoor::log::Logger::error("Unsupported --dashboard-refresh-count value: " + cliDashboardRefreshCount);
            return 1;
        }
        config.dashboardRefreshCount = parsed;
    }

    if (!cliDashboardRefreshIntervalMs.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliDashboardRefreshIntervalMs, parsed)) {
            outdoor::log::Logger::error("Unsupported --dashboard-refresh-interval-ms value: " + cliDashboardRefreshIntervalMs);
            return 1;
        }
        config.dashboardRefreshIntervalMs = static_cast<std::uint32_t>(parsed);
    }

    if (cliDisableDashboard) {
        config.dashboardEnabled = false;
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

    std::string storageLastError;
    if (!prepareStorage(config, !cliStatusOutputPath.empty(), !cliDashboardOutputPath.empty(), storageLastError)) {
        outdoor::log::Logger::error("Storage initialization failed: " + storageLastError);
        return 1;
    }

    outdoor::log::Logger::info("Outdoor Core Runtime starting");
    outdoor::log::Logger::info("Config file: " + configPath);
    outdoor::log::Logger::info(std::string("Log level: ") + outdoor::log::logLevelToString(config.logLevel));
    outdoor::log::Logger::info(std::string("Storage enabled: ") + (config.storageEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Storage root path: " + config.storageRootPath);
    outdoor::log::Logger::info("Storage log file: " + outdoor::log::Logger::fileOutputPath());
    outdoor::log::Logger::info("GNSS input mode: " + config.gnssInputMode);
    outdoor::log::Logger::info("Input source: " + config.nmeaInputPath);
    outdoor::log::Logger::info("GNSS serial device: " + config.gnssSerialDevice);
    outdoor::log::Logger::info("GNSS serial baud: " + std::to_string(config.gnssSerialBaud));
    outdoor::log::Logger::info("GNSS serial capture seconds: " + std::to_string(config.gnssSerialCaptureSeconds));
    outdoor::log::Logger::info("MCU input mode: " + config.mcuInputMode);
    outdoor::log::Logger::info("MCU mock input source: " + config.mcuMockInputPath);
    outdoor::log::Logger::info("MCU serial device: " + config.mcuSerialDevice);
    outdoor::log::Logger::info("MCU serial baud: " + std::to_string(config.mcuSerialBaud));
    outdoor::log::Logger::info("MCU serial capture seconds: " + std::to_string(config.mcuSerialCaptureSeconds));
    outdoor::log::Logger::info("MCU command: " + config.mcuCommand);
    outdoor::log::Logger::info(std::string("Board IMU enabled: ") + (config.boardImuEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Board IMU source: " + config.boardImuSource);
    outdoor::log::Logger::info("Board IMU character device path: " + config.boardImuDevicePath);
    outdoor::log::Logger::info("Board IMU IIO path: " + config.boardImuIioPath);
    outdoor::log::Logger::info("Runtime status output: " + config.statusOutputPath);
    outdoor::log::Logger::info(std::string("Dashboard enabled: ") + (config.dashboardEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Dashboard output: " + config.dashboardOutputPath);
    outdoor::log::Logger::info("Dashboard output mode: " + config.dashboardOutputMode);
    outdoor::log::Logger::info("Dashboard framebuffer device: " + config.dashboardFramebufferDevice);
    outdoor::log::Logger::info("Dashboard refresh count: " + std::to_string(config.dashboardRefreshCount));
    outdoor::log::Logger::info("Dashboard refresh interval ms: " + std::to_string(config.dashboardRefreshIntervalMs));

    outdoor::runtime::RuntimeManager runtime;
    outdoor::gnss::GnssStatus gnssStatus;
    gnssStatus.source = config.gnssInputMode == "serial" ? "uart5" : "file";
    outdoor::mcu::McuStatus mcuStatus;
    outdoor::mcu::McuCommand mcuCommand {};
    if (!outdoor::mcu::parseMcuCommand(config.mcuCommand, mcuCommand)) {
        outdoor::log::Logger::error("Unsupported mcu_command value: " + config.mcuCommand);
        return 1;
    }
    outdoor::sensors::BoardImuStatus boardImuStatus;
    boardImuStatus.enabled = config.boardImuEnabled;
    boardImuStatus.source = config.boardImuSource == "iio" ? "icm20608_iio" : "icm20608_char";
    boardImuStatus.devicePath = config.boardImuSource == "iio" ? config.boardImuIioPath : config.boardImuDevicePath;
    if (config.gnssInputMode == "serial") {
        runtime.addService(std::make_unique<outdoor::services::GnssSerialService>(
            config.gnssSerialDevice,
            config.gnssSerialBaud,
            config.gnssSerialCaptureSeconds,
            gnssStatus));
    } else {
        runtime.addService(std::make_unique<outdoor::services::GnssReplayService>(config.nmeaInputPath, gnssStatus));
    }
    if (config.mcuInputMode == "serial") {
        runtime.addService(std::make_unique<outdoor::services::McuSerialService>(
            config.mcuSerialDevice,
            config.mcuSerialBaud,
            config.mcuSerialCaptureSeconds,
            mcuCommand,
            mcuStatus));
    } else {
        runtime.addService(std::make_unique<outdoor::services::McuMockService>(config.mcuMockInputPath, mcuStatus));
    }
    if (config.boardImuEnabled) {
        runtime.addService(std::make_unique<outdoor::services::Icm20608Service>(
            config.boardImuSource,
            outdoor::sensors::Icm20608CharReader(config.boardImuDevicePath),
            outdoor::sensors::Icm20608IioReader(config.boardImuIioPath),
            boardImuStatus,
            config.boardImuSampleCount,
            config.boardImuSampleIntervalMs));
    }
    if (config.dashboardEnabled) {
        runtime.addService(std::make_unique<outdoor::services::DashboardService>(
            config.dashboardOutputPath,
            config.dashboardOutputMode,
            config.dashboardFramebufferDevice,
            config.dashboardRefreshCount,
            config.dashboardRefreshIntervalMs,
            gnssStatus,
            mcuStatus,
            boardImuStatus));
    }

    outdoor::ipc::StatusPublisher statusPublisher(config.statusOutputPath);
    runtime.setGnssStatus(gnssStatus);
    runtime.setMcuStatus(mcuStatus);
    runtime.setBoardImuStatus(boardImuStatus);
    publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));

    if (!runtime.start()) {
        runtime.setGnssStatus(gnssStatus);
        runtime.setMcuStatus(mcuStatus);
        runtime.setBoardImuStatus(boardImuStatus);
        publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
        outdoor::log::Logger::error("Outdoor Core Runtime failed to start");
        return 1;
    }
    runtime.setGnssStatus(gnssStatus);
    runtime.setMcuStatus(mcuStatus);
    runtime.setBoardImuStatus(boardImuStatus);
    publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));

    if (!runtime.run()) {
        runtime.stop();
        runtime.setGnssStatus(gnssStatus);
        runtime.setMcuStatus(mcuStatus);
        runtime.setBoardImuStatus(boardImuStatus);
        publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
        outdoor::log::Logger::error("Outdoor Core Runtime failed while running");
        return 1;
    }

    runtime.stop();
    runtime.setGnssStatus(gnssStatus);
    runtime.setMcuStatus(mcuStatus);
    runtime.setBoardImuStatus(boardImuStatus);
    publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
    outdoor::log::Logger::info("Outdoor Core Runtime stopped");
    return 0;
}
