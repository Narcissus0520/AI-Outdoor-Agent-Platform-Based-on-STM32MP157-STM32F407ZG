#include "sensors/icm20608_char_reader.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace outdoor::sensors {

namespace {

constexpr double kGyroLsbPerDps = 16.4;
constexpr double kAccelLsbPerG = 2048.0;
constexpr double kTempLsbPerCelsius = 326.8;
constexpr double kTempOffsetRaw = 25.0;
constexpr std::size_t kRawValueCount = 7;

void convertRawValues(const std::array<std::int32_t, kRawValueCount>& raw, Icm20608Sample& sample)
{
    sample.gyroXDps = static_cast<double>(raw[0]) / kGyroLsbPerDps;
    sample.gyroYDps = static_cast<double>(raw[1]) / kGyroLsbPerDps;
    sample.gyroZDps = static_cast<double>(raw[2]) / kGyroLsbPerDps;
    sample.accelXG = static_cast<double>(raw[3]) / kAccelLsbPerG;
    sample.accelYG = static_cast<double>(raw[4]) / kAccelLsbPerG;
    sample.accelZG = static_cast<double>(raw[5]) / kAccelLsbPerG;
    sample.temperatureCelsius = ((static_cast<double>(raw[6]) - kTempOffsetRaw) / kTempLsbPerCelsius) + 25.0;
}

} // namespace

Icm20608CharReader::Icm20608CharReader(std::string devicePath)
    : devicePath_(std::move(devicePath)) {}

bool Icm20608CharReader::read(Icm20608Sample& sample, std::string& error) const
{
    std::array<std::int32_t, kRawValueCount> raw {};

#ifdef _WIN32
    std::ifstream stream(devicePath_, std::ios::binary);
    if (!stream.is_open()) {
        error = "failed to open ICM20608 character device file: " + devicePath_;
        return false;
    }

    stream.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size() * sizeof(raw[0])));
    if (stream.gcount() != static_cast<std::streamsize>(raw.size() * sizeof(raw[0]))) {
        error = "failed to read complete ICM20608 character sample: " + devicePath_;
        return false;
    }
#else
    const int fd = ::open(devicePath_.c_str(), O_RDONLY);
    if (fd < 0) {
        error = "failed to open ICM20608 character device: " + devicePath_;
        return false;
    }

    const ssize_t ret = ::read(fd, raw.data(), raw.size() * sizeof(raw[0]));
    const int closeRet = ::close(fd);
    if (closeRet != 0) {
        error = "failed to close ICM20608 character device: " + devicePath_;
        return false;
    }

    // The ALIENTEK example driver copies the 7 integers but returns 0 from read().
    if (ret < 0 || (ret != 0 && ret != static_cast<ssize_t>(raw.size() * sizeof(raw[0])))) {
        error = "failed to read ICM20608 character sample: " + devicePath_;
        return false;
    }
#endif

    convertRawValues(raw, sample);
    error.clear();
    return true;
}

const std::string& Icm20608CharReader::devicePath() const
{
    return devicePath_;
}

} // namespace outdoor::sensors
