#include "services/gnss_replay_service.h"

#include "gnss/gnss_status.h"
#include "gnss/nmea_parser.h"
#include "log/logger.h"

#include <utility>

namespace outdoor::services {

namespace {

std::string sentenceType(const std::string& line)
{
    if (line.size() < 6 || line[0] != '$') {
        return "unknown";
    }
    return line.substr(3, 3);
}

} // namespace

GnssReplayService::GnssReplayService(std::string nmeaInputPath, outdoor::gnss::GnssStatus& status)
    : input_(std::move(nmeaInputPath)),
      status_(status)
{
    status_.enabled = true;
    status_.source = "file";
}

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

outdoor::runtime::ServicePollResult GnssReplayService::poll()
{
    std::string line;
    if (!input_.readLine(line)) {
        return outdoor::runtime::ServicePollResult::Completed;
    }

    if (!line.empty()) {
        outdoor::log::Logger::info("NMEA: " + line);
        status_.lastSentenceType = sentenceType(line);

        std::string parseError;
        if (parser_.parseLine(line, status_.fix, parseError)) {
            status_.seen = true;
            ++status_.validSentenceCount;
            status_.lastError.clear();
            outdoor::log::Logger::info(outdoor::gnss::formatGnssFix(status_.fix));
        } else if (!parseError.empty()) {
            ++status_.skippedSentenceCount;
            status_.lastError = parseError;
            outdoor::log::Logger::debug("NMEA parse skipped: " + parseError);
        }
    }

    return outdoor::runtime::ServicePollResult::Running;
}

void GnssReplayService::stop()
{
    input_.close();
}

} // namespace outdoor::services
