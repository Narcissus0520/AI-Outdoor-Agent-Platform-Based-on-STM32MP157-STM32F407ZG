#include "input/file_replay_input.h"
#include "log/logger.h"

#include <string>

namespace {

constexpr const char* kDefaultNmeaFile = "data/nmea_sample.txt";

} // namespace

int main(int argc, char* argv[])
{
    const std::string inputPath = (argc > 1) ? argv[1] : kDefaultNmeaFile;

    outdoor::log::Logger::info("Outdoor Core Runtime starting");
    outdoor::log::Logger::info("Input source: " + inputPath);

    outdoor::input::FileReplayInput input(inputPath);
    if (!input.open()) {
        outdoor::log::Logger::error("Failed to open NMEA input file: " + inputPath);
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
