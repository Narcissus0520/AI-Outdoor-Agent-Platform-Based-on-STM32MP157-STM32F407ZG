#include "services/gnss_replay_service.h"

#include "gnss/nmea_parser.h"
#include "log/logger.h"

#include <utility>

namespace outdoor::services {

GnssReplayService::GnssReplayService(std::string nmeaInputPath)
    : input_(std::move(nmeaInputPath)) {}

const char* GnssReplayService::name() const
{
    return "gnss_replay_service";
}

bool GnssReplayService::start()
{
    if (!input_.open()) {
        outdoor::log::Logger::error("Failed to open NMEA input file: " + input_.filePath());
        return false;
    }

    outdoor::log::Logger::info("GNSS replay input opened: " + input_.filePath());
    return true;
}

bool GnssReplayService::run()
{
    std::string line;
    while (input_.readLine(line)) {
        if (line.empty()) {
            continue;
        }

        outdoor::log::Logger::info("NMEA: " + line);

        std::string parseError;
        if (parser_.parseLine(line, currentFix_, parseError)) {
            outdoor::log::Logger::info(outdoor::gnss::formatGnssFix(currentFix_));
        } else if (!parseError.empty()) {
            outdoor::log::Logger::debug("NMEA parse skipped: " + parseError);
        }
    }

    return true;
}

void GnssReplayService::stop()
{
    input_.close();
}

} // namespace outdoor::services
