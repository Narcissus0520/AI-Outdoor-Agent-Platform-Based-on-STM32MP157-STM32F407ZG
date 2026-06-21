#pragma once

#include <string>

namespace outdoor::gnss {

struct GnssFix {
    bool valid = false;
    double latitudeDegrees = 0.0;
    double longitudeDegrees = 0.0;
    double altitudeMeters = 0.0;
    double speedKnots = 0.0;
    double speedKmh = 0.0;
    double courseDegrees = 0.0;
    int satelliteCount = 0;
    int satellitesInView = 0;
    int fixQuality = 0;
    int fixType = 0;
    double hdop = 0.0;
    double pdop = 0.0;
    double vdop = 0.0;
    std::string utcTime;
};

} // namespace outdoor::gnss
