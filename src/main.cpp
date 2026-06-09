#include "config/config_loader.h"
#include "input/file_replay_input.h"
#include "log/logger.h"

#include <iostream>
#include <string>

namespace {

constexpr const char* kDefaultNmeaFile = "data/nmea_sample.txt";
constexpr const char* kDefaultConfigFile = "config/runtime.conf";

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [nmea_file]\n"
              << "       " << programName << " [--config path] [--input path] [--log-level debug|info|warn|error]\n";
}

} // namespace

int main(int argc, char* argv[])
{
    outdoor::config::AppConfig config;
    config.nmeaInputPath = kDefaultNmeaFile;

    std::string configPath = kDefaultConfigFile;
    std::string cliInputPath;
    std::string cliLogLevel;

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

    outdoor::input::FileReplayInput input(config.nmeaInputPath);
    if (!input.open()) {
        outdoor::log::Logger::error("Failed to open NMEA input file: " + config.nmeaInputPath);
        return 1;
    }

    std::string line;
    while (input.readLine(line)) {
        if (line.empty()) {
            continue;
        }

        outdoor::log::Logger::info("NMEA: " + line);
    }

    input.close();
    outdoor::log::Logger::info("Outdoor Core Runtime stopped");
    return 0;
}
