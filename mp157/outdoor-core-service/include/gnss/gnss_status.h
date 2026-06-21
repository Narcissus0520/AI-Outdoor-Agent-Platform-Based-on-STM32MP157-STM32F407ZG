#pragma once

#include "gnss/gnss_fix.h"

#include <cstdint>
#include <string>

namespace outdoor::gnss {

struct GnssStatus {
    bool enabled = true;
    bool seen = false;
    std::string source = "file";
    std::uint32_t validSentenceCount = 0;
    std::uint32_t skippedSentenceCount = 0;
    std::string lastSentenceType = "none";
    std::string lastError;
    GnssFix fix;
};

std::string formatGnssStatus(const GnssStatus& status);

} // namespace outdoor::gnss
