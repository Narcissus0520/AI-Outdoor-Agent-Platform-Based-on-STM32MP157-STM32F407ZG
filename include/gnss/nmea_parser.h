#pragma once

#include "gnss/gnss_fix.h"

#include <string>

namespace outdoor::gnss {

class NmeaParser {
public:
    bool parseLine(const std::string& line, GnssFix& fix, std::string& error) const;

private:
    bool parseRmc(const std::string& line, GnssFix& fix, std::string& error) const;
    bool parseGga(const std::string& line, GnssFix& fix, std::string& error) const;
};

std::string formatGnssFix(const GnssFix& fix);

} // namespace outdoor::gnss
