#include "config/config_loader.h"
#include "ipc/status_publisher.h"
#include "log/logger.h"
#include "runtime/runtime_manager.h"
#include "services/gnss_replay_service.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

constexpr const char* kDefaultNmeaFile = "data/nmea_sample.txt";
constexpr const char* kDefaultConfigFile = "config/runtime.conf";
constexpr const char* kDefaultStatusOutputFile = "runtime/status.txt";

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [nmea_file]\n"
              << "       " << programName
              << " [--config path] [--input path] [--status-output path]"
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

} // namespace

int main(int argc, char* argv[])
{
    outdoor::config::AppConfig config;
    config.nmeaInputPath = kDefaultNmeaFile;
    config.statusOutputPath = kDefaultStatusOutputFile;

    std::string configPath = kDefaultConfigFile;
    std::string cliInputPath;
    std::string cliLogLevel;
    std::string cliStatusOutputPath;

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
        } else if (arg == "--status-output") {
            if (index + 1 >= argc) {
                outdoor::log::Logger::error("--status-output requires a file path");
                return 1;
            }
            cliStatusOutputPath = argv[++index];
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

    if (!cliStatusOutputPath.empty()) {
        config.statusOutputPath = cliStatusOutputPath;
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
    outdoor::log::Logger::info("Runtime status output: " + config.statusOutputPath);

    outdoor::runtime::RuntimeManager runtime;
    runtime.addService(std::make_unique<outdoor::services::GnssReplayService>(config.nmeaInputPath));

    outdoor::ipc::StatusPublisher statusPublisher(config.statusOutputPath);
    publishStatus(statusPublisher, runtime.status());

    if (!runtime.start()) {
        publishStatus(statusPublisher, runtime.status());
        outdoor::log::Logger::error("Outdoor Core Runtime failed to start");
        return 1;
    }
    publishStatus(statusPublisher, runtime.status());

    if (!runtime.run()) {
        runtime.stop();
        publishStatus(statusPublisher, runtime.status());
        outdoor::log::Logger::error("Outdoor Core Runtime failed while running");
        return 1;
    }

    runtime.stop();
    publishStatus(statusPublisher, runtime.status());
    outdoor::log::Logger::info("Outdoor Core Runtime stopped");
    return 0;
}
