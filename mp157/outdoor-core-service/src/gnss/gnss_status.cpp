#include "gnss/gnss_status.h"

#include "gnss/nmea_parser.h"

#include <sstream>

namespace outdoor::gnss {

std::string formatGnssStatus(const GnssStatus& status)
{
    std::ostringstream stream;
    stream << "GNSS status: enabled=" << (status.enabled ? "true" : "false")
           << ", seen=" << (status.seen ? "true" : "false")
           << ", source=" << status.source
           << ", valid_sentences=" << status.validSentenceCount
           << ", skipped_sentences=" << status.skippedSentenceCount
           << ", last_sentence_type=" << status.lastSentenceType
           << ", " << formatGnssFix(status.fix);
    if (!status.lastError.empty()) {
        stream << ", last_error=" << status.lastError;
    }
    return stream.str();
}

} // namespace outdoor::gnss
