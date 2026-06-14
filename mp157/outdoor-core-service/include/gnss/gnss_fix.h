#pragma once

#include <string>

namespace outdoor::gnss {

struct GnssFix {
    bool valid = false;
    double latitudeDegrees = 0.0;
    double longitudeDegrees = 0.0;
    double altitudeMeters = 0.0;
    double speedKnots = 0.0;
    int satelliteCount = 0;
    int fixQuality = 0;
    std::string utcTime;
};

} // namespace outdoor::gnss
