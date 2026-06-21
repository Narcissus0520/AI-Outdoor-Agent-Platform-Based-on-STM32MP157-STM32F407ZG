#include "gnss/nmea_parser.h"

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace outdoor::gnss {

namespace {

std::vector<std::string> splitFields(std::string payload)
{
    const auto checksumPosition = payload.find('*');
    if (checksumPosition != std::string::npos) {
        payload = payload.substr(0, checksumPosition);
    }

    std::vector<std::string> fields;
    std::stringstream stream(payload);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(field);
    }

    return fields;
}

std::string sentenceType(const std::string& line)
{
    if (line.size() < 6 || line[0] != '$') {
        return {};
    }

    return line.substr(3, 3);
}

bool parseDouble(const std::string& value, double& output)
{
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    output = std::strtod(value.c_str(), &end);
    return end != value.c_str() && *end == '\0';
}

bool parseInt(const std::string& value, int& output)
{
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    const long parsedValue = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }

    output = static_cast<int>(parsedValue);
    return true;
}

int hexValue(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }

    return -1;
}

bool validateChecksum(const std::string& line, std::string& error)
{
    if (line.empty() || line[0] != '$') {
        error = "NMEA sentence does not start with '$'";
        return false;
    }

    const auto checksumPosition = line.find('*');
    if (checksumPosition == std::string::npos) {
        error = "NMEA sentence missing checksum";
        return false;
    }

    if (checksumPosition + 2 >= line.size()) {
        error = "NMEA checksum is too short";
        return false;
    }

    const int high = hexValue(line[checksumPosition + 1]);
    const int low = hexValue(line[checksumPosition + 2]);
    if (high < 0 || low < 0) {
        error = "NMEA checksum contains non-hex characters";
        return false;
    }

    unsigned char actual = 0;
    for (std::size_t index = 1; index < checksumPosition; ++index) {
        actual ^= static_cast<unsigned char>(line[index]);
    }

    const unsigned char expected = static_cast<unsigned char>((high << 4) | low);
    if (actual != expected) {
        std::ostringstream message;
        message << "NMEA checksum mismatch: expected "
                << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(expected)
                << ", calculated "
                << std::setw(2) << static_cast<int>(actual);
        error = message.str();
        return false;
    }

    return true;
}

bool parseCoordinate(const std::string& value, const std::string& hemisphere, double& degrees)
{
    double raw = 0.0;
    if (!parseDouble(value, raw) || hemisphere.empty()) {
        return false;
    }

    const int degreePart = static_cast<int>(raw / 100.0);
    const double minutePart = raw - static_cast<double>(degreePart * 100);
    degrees = static_cast<double>(degreePart) + minutePart / 60.0;

    if (hemisphere == "S" || hemisphere == "W") {
        degrees = -degrees;
    } else if (hemisphere != "N" && hemisphere != "E") {
        return false;
    }

    return true;
}

} // namespace

bool NmeaParser::parseLine(const std::string& line, GnssFix& fix, std::string& error) const
{
    error.clear();
    if (!validateChecksum(line, error)) {
        return false;
    }

    const std::string type = sentenceType(line);
    if (type == "RMC") {
        return parseRmc(line, fix, error);
    }
    if (type == "GGA") {
        return parseGga(line, fix, error);
    }
    if (type == "VTG") {
        return parseVtg(line, fix, error);
    }
    if (type == "GSA") {
        return parseGsa(line, fix, error);
    }
    if (type == "GSV") {
        return parseGsv(line, fix, error);
    }

    return false;
}

bool NmeaParser::parseRmc(const std::string& line, GnssFix& fix, std::string& error) const
{
    const auto fields = splitFields(line);
    if (fields.size() < 8) {
        error = "RMC sentence has too few fields";
        return false;
    }

    if (fields[2] != "A") {
        fix.valid = false;
        error = "RMC status is not active";
        return false;
    }

    double latitude = 0.0;
    double longitude = 0.0;
    if (!parseCoordinate(fields[3], fields[4], latitude) ||
        !parseCoordinate(fields[5], fields[6], longitude)) {
        error = "RMC coordinate parse failed";
        return false;
    }

    double speed = 0.0;
    if (!fields[7].empty() && !parseDouble(fields[7], speed)) {
        error = "RMC speed parse failed";
        return false;
    }

    fix.valid = true;
    fix.utcTime = fields[1];
    fix.latitudeDegrees = latitude;
    fix.longitudeDegrees = longitude;
    fix.speedKnots = speed;
    fix.speedKmh = speed * 1.852;
    return true;
}

bool NmeaParser::parseGga(const std::string& line, GnssFix& fix, std::string& error) const
{
    const auto fields = splitFields(line);
    if (fields.size() < 10) {
        error = "GGA sentence has too few fields";
        return false;
    }

    int fixQuality = 0;
    if (!parseInt(fields[6], fixQuality) || fixQuality <= 0) {
        fix.valid = false;
        error = "GGA fix quality is not valid";
        return false;
    }

    double latitude = 0.0;
    double longitude = 0.0;
    if (!parseCoordinate(fields[2], fields[3], latitude) ||
        !parseCoordinate(fields[4], fields[5], longitude)) {
        error = "GGA coordinate parse failed";
        return false;
    }

    int satellites = 0;
    if (!parseInt(fields[7], satellites)) {
        error = "GGA satellite count parse failed";
        return false;
    }

    double altitude = 0.0;
    if (!parseDouble(fields[9], altitude)) {
        error = "GGA altitude parse failed";
        return false;
    }

    fix.valid = true;
    fix.utcTime = fields[1];
    fix.latitudeDegrees = latitude;
    fix.longitudeDegrees = longitude;
    fix.fixQuality = fixQuality;
    fix.satelliteCount = satellites;
    fix.altitudeMeters = altitude;
    if (fields.size() > 8) {
        double hdop = 0.0;
        if (!fields[8].empty() && parseDouble(fields[8], hdop)) {
            fix.hdop = hdop;
        }
    }
    return true;
}

bool NmeaParser::parseVtg(const std::string& line, GnssFix& fix, std::string& error) const
{
    const auto fields = splitFields(line);
    if (fields.size() < 9) {
        error = "VTG sentence has too few fields";
        return false;
    }

    double course = 0.0;
    if (!fields[1].empty()) {
        if (!parseDouble(fields[1], course)) {
            error = "VTG course parse failed";
            return false;
        }
        fix.courseDegrees = course;
    }

    double speedKnots = 0.0;
    if (!fields[5].empty()) {
        if (!parseDouble(fields[5], speedKnots)) {
            error = "VTG speed knots parse failed";
            return false;
        }
        fix.speedKnots = speedKnots;
    }

    double speedKmh = 0.0;
    if (!fields[7].empty()) {
        if (!parseDouble(fields[7], speedKmh)) {
            error = "VTG speed kmh parse failed";
            return false;
        }
        fix.speedKmh = speedKmh;
    } else {
        fix.speedKmh = fix.speedKnots * 1.852;
    }

    return true;
}

bool NmeaParser::parseGsa(const std::string& line, GnssFix& fix, std::string& error) const
{
    const auto fields = splitFields(line);
    if (fields.size() < 18) {
        error = "GSA sentence has too few fields";
        return false;
    }

    int fixType = 0;
    if (!parseInt(fields[2], fixType)) {
        error = "GSA fix type parse failed";
        return false;
    }
    fix.fixType = fixType;

    double pdop = 0.0;
    double hdop = 0.0;
    double vdop = 0.0;
    if (!fields[15].empty() && !parseDouble(fields[15], pdop)) {
        error = "GSA PDOP parse failed";
        return false;
    }
    if (!fields[16].empty() && !parseDouble(fields[16], hdop)) {
        error = "GSA HDOP parse failed";
        return false;
    }
    if (!fields[17].empty() && !parseDouble(fields[17], vdop)) {
        error = "GSA VDOP parse failed";
        return false;
    }
    fix.pdop = pdop;
    fix.hdop = hdop;
    fix.vdop = vdop;
    return true;
}

bool NmeaParser::parseGsv(const std::string& line, GnssFix& fix, std::string& error) const
{
    const auto fields = splitFields(line);
    if (fields.size() < 4) {
        error = "GSV sentence has too few fields";
        return false;
    }

    int satellitesInView = 0;
    if (!parseInt(fields[3], satellitesInView)) {
        error = "GSV satellites in view parse failed";
        return false;
    }
    fix.satellitesInView = satellitesInView;
    return true;
}

std::string formatGnssFix(const GnssFix& fix)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6)
           << "GNSS fix: valid=" << (fix.valid ? "true" : "false")
           << ", lat=" << fix.latitudeDegrees
           << ", lon=" << fix.longitudeDegrees
           << std::setprecision(1)
           << ", alt_m=" << fix.altitudeMeters
           << std::setprecision(3)
           << ", speed_knots=" << fix.speedKnots
           << std::setprecision(1)
           << ", speed_kmh=" << fix.speedKmh
           << ", course_deg=" << fix.courseDegrees
           << ", satellites=" << fix.satelliteCount
           << ", satellites_in_view=" << fix.satellitesInView
           << ", fix_quality=" << fix.fixQuality
           << ", fix_type=" << fix.fixType
           << ", hdop=" << fix.hdop
           << ", pdop=" << fix.pdop
           << ", vdop=" << fix.vdop
           << ", utc=" << fix.utcTime;
    return stream.str();
}

} // namespace outdoor::gnss
