#include "config/config_loader.h"
#include "gnss/gnss_status.h"
#include "ipc/status_publisher.h"
#include "ipc/unix_status_service.h"
#include "log/logger.h"
#include "mcu/mcu_command.h"
#include "mcu/mcu_status.h"
#include "navigation/compass_estimator.h"
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
#include "storage/history_recorder.h"

#include <cstddef>
#include <cstdint>
#include <csignal>
#include <chrono>
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
constexpr std::uint16_t kCompassRequiredMcuFlags = 0x0009U;
constexpr std::uint16_t kCompassRejectedMcuFlags = 0x5216U;
volatile std::sig_atomic_t shutdownRequested = 0;

void handleShutdownSignal(int)
{
    shutdownRequested = 1;
}

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
              << " [--runtime-run-seconds seconds]"
              << " [--status-output path]"
              << " [--status-socket] [--no-status-socket] [--status-socket-path path]"
              << " [--storage] [--no-storage] [--storage-root path]"
              << " [--storage-status-output path] [--storage-dashboard-output path]"
              << " [--storage-log-file path]"
              << " [--history] [--no-history] [--history-output path]"
              << " [--board-imu] [--board-imu-source char_device|iio|auto]"
              << " [--board-imu-device-path path] [--board-imu-iio-path path] [--board-imu-samples count]"
              << " [--dashboard-output path] [--dashboard-output-mode text|framebuffer|both]"
              << " [--dashboard-framebuffer-device path]"
              << " [--dashboard-refresh-count count] [--dashboard-refresh-interval-ms ms]"
              << " [--no-dashboard]"
              << " [--launcher] [--no-launcher] [--launcher-input-device path|auto]"
              << " [--launcher-auto-start-seconds seconds]"
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
        if (!outdoor::log::Logger::setFileOutput(logFilePath,
                                                 config.storageLogMaxBytes,
                                                 config.storageLogBackupCount,
                                                 error)) {
            return false;
        }
        config.historyOutputPath = resolveStoragePath(config.storageRootPath, config.historyOutputPath);
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
    const outdoor::log::LoggerStatus loggerStatus = outdoor::log::Logger::status();
    status.storageEnabled = config.storageEnabled;
    status.statusSocketEnabled = config.statusSocketEnabled;
    status.statusSocketPath = config.statusSocketPath;
    status.storageRootPath = config.storageRootPath;
    status.storageStatusOutputPath = config.statusOutputPath;
    status.storageDashboardOutputPath = config.dashboardOutputPath;
    status.storageLogFilePath = outdoor::log::Logger::fileOutputPath();
    status.storageLogMaxBytes = config.storageLogMaxBytes;
    status.storageLogBackupCount = config.storageLogBackupCount;
    status.storageLogRotationFailureCount = loggerStatus.rotationFailureCount;
    status.storageLogWriteFailureCount = loggerStatus.writeFailureCount;
    status.storageLogLastError = loggerStatus.lastError;
    status.historyEnabled = config.historyEnabled;
    status.historyOutputPath = config.historyOutputPath;
    status.storageLastError = !storageLastError.empty() ? storageLastError : loggerStatus.lastError;
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
    (void)std::signal(SIGINT, handleShutdownSignal);
    (void)std::signal(SIGTERM, handleShutdownSignal);

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
    std::string cliRuntimeRunSeconds;
    std::string cliLogLevel;
    std::string cliStatusOutputPath;
    bool cliEnableStatusSocket = false;
    bool cliDisableStatusSocket = false;
    std::string cliStatusSocketPath;
    bool cliEnableStorage = false;
    bool cliDisableStorage = false;
    std::string cliStorageRootPath;
    std::string cliStorageStatusOutputPath;
    std::string cliStorageDashboardOutputPath;
    std::string cliStorageLogFilePath;
    bool cliEnableHistory = false;
    bool cliDisableHistory = false;
    std::string cliHistoryOutputPath;
    bool cliEnableBoardImu = false;
    bool cliDisableBoardImu = false;
    bool cliDisableDashboard = false;
    std::string cliDashboardOutputPath;
    std::string cliDashboardOutputMode;
    std::string cliDashboardFramebufferDevice;
    std::string cliDashboardRefreshCount;
    std::string cliDashboardRefreshIntervalMs;
    bool cliEnableLauncher = false;
    bool cliDisableLauncher = false;
    std::string cliLauncherInputDevice;
    std::string cliLauncherAutoStartSeconds;
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
        } else if (arg == "--runtime-run-seconds") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--runtime-run-seconds requires seconds");
                return 1;
            }
            cliRuntimeRunSeconds = argv[++index];
        } else if (arg == "--status-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--status-output requires a file path");
                return 1;
            }
            cliStatusOutputPath = argv[++index];
        } else if (arg == "--status-socket") {
            cliEnableStatusSocket = true;
            cliDisableStatusSocket = false;
        } else if (arg == "--no-status-socket") {
            cliDisableStatusSocket = true;
            cliEnableStatusSocket = false;
        } else if (arg == "--status-socket-path") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--status-socket-path requires a file path");
                return 1;
            }
            cliStatusSocketPath = argv[++index];
            cliEnableStatusSocket = true;
            cliDisableStatusSocket = false;
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
        } else if (arg == "--history") {
            cliEnableHistory = true;
            cliDisableHistory = false;
        } else if (arg == "--no-history") {
            cliDisableHistory = true;
            cliEnableHistory = false;
        } else if (arg == "--history-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--history-output requires a directory path");
                return 1;
            }
            cliHistoryOutputPath = argv[++index];
            cliEnableHistory = true;
            cliDisableHistory = false;
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
        } else if (arg == "--launcher") {
            cliEnableLauncher = true;
            cliDisableLauncher = false;
        } else if (arg == "--no-launcher") {
            cliDisableLauncher = true;
            cliEnableLauncher = false;
        } else if (arg == "--launcher-input-device") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--launcher-input-device requires a device path or auto");
                return 1;
            }
            cliLauncherInputDevice = argv[++index];
        } else if (arg == "--launcher-auto-start-seconds") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--launcher-auto-start-seconds requires seconds");
                return 1;
            }
            cliLauncherAutoStartSeconds = argv[++index];
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

    if (!cliRuntimeRunSeconds.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliRuntimeRunSeconds, parsed)) {
            outdoor::log::Logger::error("Unsupported --runtime-run-seconds value: " + cliRuntimeRunSeconds);
            return 1;
        }
        config.runtimeRunSeconds = static_cast<std::uint32_t>(parsed);
    }

    if (!cliStatusOutputPath.empty()) {
        config.statusOutputPath = cliStatusOutputPath;
    }

    if (cliEnableStatusSocket) {
        config.statusSocketEnabled = true;
    }

    if (cliDisableStatusSocket) {
        config.statusSocketEnabled = false;
    }

    if (!cliStatusSocketPath.empty()) {
        config.statusSocketPath = cliStatusSocketPath;
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

    if (cliEnableHistory) {
        config.historyEnabled = true;
    }

    if (cliDisableHistory) {
        config.historyEnabled = false;
    }

    if (!cliHistoryOutputPath.empty()) {
        config.historyOutputPath = cliHistoryOutputPath;
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

    if (cliEnableLauncher) {
        config.launcherEnabled = true;
    }

    if (cliDisableLauncher) {
        config.launcherEnabled = false;
    }

    if (!cliLauncherInputDevice.empty()) {
        config.launcherInputDevice = cliLauncherInputDevice;
    }

    if (!cliLauncherAutoStartSeconds.empty()) {
        std::size_t parsed = 0;
        if (!parseSizeArgument(cliLauncherAutoStartSeconds, parsed)) {
            outdoor::log::Logger::error("Unsupported --launcher-auto-start-seconds value: " + cliLauncherAutoStartSeconds);
            return 1;
        }
        config.launcherAutoStartSeconds = static_cast<std::uint32_t>(parsed);
    }

    if (!cliLogLevel.empty()) {
        outdoor::log::LogLevel parsedLevel {};
        if (!outdoor::log::parseLogLevel(cliLogLevel, parsedLevel)) {
            outdoor::log::Logger::error("Unsupported --log-level value: " + cliLogLevel);
            return 1;
        }
        config.logLevel = parsedLevel;
    }

    std::string compassConfigError;
    if (!outdoor::navigation::validateCompassConfig(config.compass, compassConfigError)) {
        outdoor::log::Logger::error("Invalid compass configuration: " + compassConfigError);
        return 1;
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
    outdoor::log::Logger::info("Storage log max bytes: " + std::to_string(config.storageLogMaxBytes));
    outdoor::log::Logger::info("Storage log backup count: " + std::to_string(config.storageLogBackupCount));
    outdoor::log::Logger::info(std::string("History enabled: ") + (config.historyEnabled ? "true" : "false"));
    outdoor::log::Logger::info("History output path: " + config.historyOutputPath);
    outdoor::log::Logger::info(std::string("Compass enabled: ")
                               + (config.compass.enabled ? "true" : "false"));
    outdoor::log::Logger::info(std::string("Compass calibration valid: ")
                               + (config.compass.calibrationValid ? "true" : "false"));
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
    outdoor::log::Logger::info("Runtime run seconds: " + std::to_string(config.runtimeRunSeconds));
    outdoor::log::Logger::info(std::string("Board IMU enabled: ") + (config.boardImuEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Board IMU source: " + config.boardImuSource);
    outdoor::log::Logger::info("Board IMU character device path: " + config.boardImuDevicePath);
    outdoor::log::Logger::info("Board IMU IIO path: " + config.boardImuIioPath);
    outdoor::log::Logger::info("Runtime status output: " + config.statusOutputPath);
    outdoor::log::Logger::info(std::string("Runtime status socket enabled: ")
                               + (config.statusSocketEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Runtime status socket path: " + config.statusSocketPath);
    outdoor::log::Logger::info(std::string("Dashboard enabled: ") + (config.dashboardEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Dashboard output: " + config.dashboardOutputPath);
    outdoor::log::Logger::info("Dashboard output mode: " + config.dashboardOutputMode);
    outdoor::log::Logger::info("Dashboard framebuffer device: " + config.dashboardFramebufferDevice);
    outdoor::log::Logger::info("Dashboard refresh count: " + std::to_string(config.dashboardRefreshCount));
    outdoor::log::Logger::info("Dashboard refresh interval ms: " + std::to_string(config.dashboardRefreshIntervalMs));
    outdoor::log::Logger::info(std::string("Launcher enabled: ") + (config.launcherEnabled ? "true" : "false"));
    outdoor::log::Logger::info("Launcher input device: " + config.launcherInputDevice);
    outdoor::log::Logger::info("Launcher auto-start seconds: " + std::to_string(config.launcherAutoStartSeconds));

    outdoor::runtime::RuntimeManager runtime;
    outdoor::gnss::GnssStatus gnssStatus;
    gnssStatus.source = config.gnssInputMode == "serial" ? "uart5" : "file";
    outdoor::mcu::McuStatus mcuStatus;
    outdoor::navigation::CompassEstimator compassEstimator(config.compass);
    const auto updateCompassStatus = [&]() {
        const bool compassSourcesHealthy = mcuStatus.heartbeatSeen
            && (mcuStatus.statusFlags & kCompassRequiredMcuFlags) == kCompassRequiredMcuFlags
            && (mcuStatus.statusFlags & kCompassRejectedMcuFlags) == 0U;
        mcuStatus.compassStatus = compassEstimator.estimate(
            mcuStatus.imuSeen,
            mcuStatus.imuSample,
            mcuStatus.magnetometerSeen,
            mcuStatus.magnetometerSample,
            compassSourcesHealthy);
    };
    updateCompassStatus();
    outdoor::mcu::McuCommand mcuCommand {};
    if (!outdoor::mcu::parseMcuCommand(config.mcuCommand, mcuCommand)) {
        outdoor::log::Logger::error("Unsupported mcu_command value: " + config.mcuCommand);
        return 1;
    }
    outdoor::sensors::BoardImuStatus boardImuStatus;
    boardImuStatus.enabled = config.boardImuEnabled;
    boardImuStatus.source = config.boardImuSource == "iio" ? "icm20608_iio" : "icm20608_char";
    boardImuStatus.devicePath = config.boardImuSource == "iio" ? config.boardImuIioPath : config.boardImuDevicePath;

    std::unique_ptr<outdoor::storage::HistoryRecorder> historyRecorder;
    if (config.historyEnabled) {
        historyRecorder = std::make_unique<outdoor::storage::HistoryRecorder>(
            config.historyOutputPath,
            config.historyFlushIntervalMs,
            gnssStatus,
            mcuStatus,
            boardImuStatus);
        std::string historyError;
        if (!historyRecorder->start(historyError)) {
            outdoor::log::Logger::error("History recorder initialization failed: " + historyError);
            return 1;
        }
    }

    const auto recordHistoryUpdate = [&historyRecorder](std::string& error) {
        if (!historyRecorder) {
            error.clear();
            return true;
        }
        return historyRecorder->recordSnapshot(error);
    };
    const auto recordMcuUpdate = [&updateCompassStatus, &recordHistoryUpdate](std::string& error) {
        updateCompassStatus();
        return recordHistoryUpdate(error);
    };

    if (config.gnssInputMode == "serial") {
        runtime.addService(std::make_unique<outdoor::services::GnssSerialService>(
            config.gnssSerialDevice,
            config.gnssSerialBaud,
            config.gnssSerialCaptureSeconds,
            gnssStatus,
            recordHistoryUpdate));
    } else {
        runtime.addService(std::make_unique<outdoor::services::GnssReplayService>(config.nmeaInputPath, gnssStatus));
    }
    if (config.mcuInputMode == "serial") {
        runtime.addService(std::make_unique<outdoor::services::McuSerialService>(
            config.mcuSerialDevice,
            config.mcuSerialBaud,
            config.mcuSerialCaptureSeconds,
            mcuCommand,
            mcuStatus,
            recordMcuUpdate));
    } else {
        runtime.addService(std::make_unique<outdoor::services::McuMockService>(
            config.mcuMockInputPath,
            mcuStatus,
            recordMcuUpdate));
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
            config.launcherEnabled,
            config.launcherInputDevice,
            config.launcherAutoStartSeconds,
            gnssStatus,
            mcuStatus,
            boardImuStatus));
    }

    outdoor::ipc::StatusPublisher statusPublisher(config.statusOutputPath);
    const auto syncRuntimeStatus = [&]() {
        updateCompassStatus();
        runtime.setGnssStatus(gnssStatus);
        runtime.setMcuStatus(mcuStatus);
        runtime.setBoardImuStatus(boardImuStatus);
    };
    const auto currentStatusJson = [&]() {
        syncRuntimeStatus();
        return outdoor::ipc::StatusPublisher::serialize(
            buildStatus(runtime, config, storageLastError));
    };
    if (config.statusSocketEnabled) {
        runtime.addService(std::make_unique<outdoor::ipc::UnixStatusService>(
            config.statusSocketPath,
            currentStatusJson));
    }
    syncRuntimeStatus();
    publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));

    const auto runtimeStartedAt = std::chrono::steady_clock::now();
    runtime.setStopPredicate([&config, runtimeStartedAt]() {
        if (shutdownRequested != 0) {
            return true;
        }
        if (config.runtimeRunSeconds == 0U) {
            return false;
        }
        return std::chrono::steady_clock::now() - runtimeStartedAt
            >= std::chrono::seconds(config.runtimeRunSeconds);
    });
    auto nextStatusPublishAt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    runtime.setLoopCallback([&]() {
        updateCompassStatus();
        if (historyRecorder) {
            std::string historyError;
            if (!historyRecorder->recordSnapshot(historyError)) {
                throw std::runtime_error(historyError);
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= nextStatusPublishAt) {
            syncRuntimeStatus();
            publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
            nextStatusPublishAt = now + std::chrono::seconds(1);
        }
    }, 0U);

    if (!runtime.start()) {
        if (historyRecorder) {
            historyRecorder->stop();
        }
        outdoor::log::Logger::error("Outdoor Core Runtime failed to start");
        syncRuntimeStatus();
        publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
        return 1;
    }
    syncRuntimeStatus();
    publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));

    if (!runtime.run()) {
        runtime.stop();
        if (historyRecorder) {
            historyRecorder->stop();
        }
        outdoor::log::Logger::error("Outdoor Core Runtime failed while running");
        syncRuntimeStatus();
        publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
        return 1;
    }

    runtime.stop();
    if (historyRecorder) {
        historyRecorder->stop();
    }
    outdoor::log::Logger::info("Outdoor Core Runtime stopped");
    syncRuntimeStatus();
    publishStatus(statusPublisher, buildStatus(runtime, config, storageLastError));
    return 0;
}
