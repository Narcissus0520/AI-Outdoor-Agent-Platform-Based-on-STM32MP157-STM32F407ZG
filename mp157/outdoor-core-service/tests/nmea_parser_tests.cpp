#include "gnss/nmea_parser.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool nearlyEqual(double actual, double expected, double tolerance)
{
    return std::fabs(actual - expected) <= tolerance;
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main()
{
    outdoor::gnss::NmeaParser parser;
    outdoor::gnss::GnssFix fix;
    std::string error;

    require(parser.parseLine("$GNRMC,024603.00,A,2232.46872,N,11404.37251,E,0.043,,090626,,,A,V*17", fix, error),
            "RMC parse failed: " + error);
    require(fix.valid, "RMC did not mark fix valid");
    require(nearlyEqual(fix.latitudeDegrees, 22.541145, 0.000001), "RMC latitude mismatch");
    require(nearlyEqual(fix.longitudeDegrees, 114.072875, 0.000001), "RMC longitude mismatch");
    require(nearlyEqual(fix.speedKmh, 0.079636, 0.0001), "RMC speed kmh mismatch");

    require(parser.parseLine("$GNGGA,024603.00,2232.46872,N,11404.37251,E,1,12,0.78,47.3,M,-2.7,M,,*69", fix, error),
            "GGA parse failed: " + error);
    require(fix.satelliteCount == 12, "GGA satellite count mismatch");
    require(fix.fixQuality == 1, "GGA fix quality mismatch");
    require(nearlyEqual(fix.altitudeMeters, 47.3, 0.001), "GGA altitude mismatch");
    require(nearlyEqual(fix.hdop, 0.78, 0.001), "GGA HDOP mismatch");

    require(parser.parseLine("$GNVTG,84.7,T,,M,1.25,N,2.32,K,A*2D", fix, error),
            "VTG parse failed: " + error);
    require(nearlyEqual(fix.courseDegrees, 84.7, 0.001), "VTG course mismatch");
    require(nearlyEqual(fix.speedKnots, 1.25, 0.001), "VTG speed knots mismatch");
    require(nearlyEqual(fix.speedKmh, 2.32, 0.001), "VTG speed kmh mismatch");

    require(parser.parseLine("$GNGSA,A,3,03,08,10,16,21,26,27,31,,,,,1.45,0.78,1.22,1*02", fix, error),
            "GSA parse failed: " + error);
    require(fix.fixType == 3, "GSA fix type mismatch");
    require(nearlyEqual(fix.pdop, 1.45, 0.001), "GSA PDOP mismatch");
    require(nearlyEqual(fix.hdop, 0.78, 0.001), "GSA HDOP mismatch");
    require(nearlyEqual(fix.vdop, 1.22, 0.001), "GSA VDOP mismatch");

    require(parser.parseLine("$GPGSV,3,1,12,03,47,083,35,08,21,312,29,10,67,174,38,16,15,042,25,1*63", fix, error),
            "GSV parse failed: " + error);
    require(fix.satellitesInView == 12, "GSV satellites in view mismatch");

    require(!parser.parseLine("$GNRMC,024603.00,A,2232.46872,N,11404.37251,E,0.043,,090626,,,A,V*00", fix, error),
            "Bad checksum sentence unexpectedly parsed");
    require(error.find("checksum") != std::string::npos, "Bad checksum did not report checksum error");

    return 0;
}
