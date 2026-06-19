#include "sensors/icm20608_iio_reader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace outdoor::sensors {

Icm20608IioReader::Icm20608IioReader(std::string devicePath)
    : devicePath_(std::move(devicePath)) {}

bool Icm20608IioReader::read(Icm20608Sample& sample, std::string& error) const
{
    double accelScale = 0.0;
    int accelXRaw = 0;
    int accelYRaw = 0;
    int accelZRaw = 0;
    double gyroScale = 0.0;
    int gyroXRaw = 0;
    int gyroYRaw = 0;
    int gyroZRaw = 0;
    double tempScale = 0.0;
    int tempOffset = 0;
    int tempRaw = 0;

    if (!readDouble("in_accel_scale", accelScale, error)
        || !readInt("in_accel_x_raw", accelXRaw, error)
        || !readInt("in_accel_y_raw", accelYRaw, error)
        || !readInt("in_accel_z_raw", accelZRaw, error)
        || !readDouble("in_anglvel_scale", gyroScale, error)
        || !readInt("in_anglvel_x_raw", gyroXRaw, error)
        || !readInt("in_anglvel_y_raw", gyroYRaw, error)
        || !readInt("in_anglvel_z_raw", gyroZRaw, error)
        || !readDouble("in_temp_scale", tempScale, error)
        || !readInt("in_temp_offset", tempOffset, error)
        || !readInt("in_temp_raw", tempRaw, error)) {
        return false;
    }

    if (tempScale == 0.0) {
        error = "ICM20608 IIO temperature scale is zero";
        return false;
    }

    sample.accelXG = static_cast<double>(accelXRaw) * accelScale;
    sample.accelYG = static_cast<double>(accelYRaw) * accelScale;
    sample.accelZG = static_cast<double>(accelZRaw) * accelScale;
    sample.gyroXDps = static_cast<double>(gyroXRaw) * gyroScale;
    sample.gyroYDps = static_cast<double>(gyroYRaw) * gyroScale;
    sample.gyroZDps = static_cast<double>(gyroZRaw) * gyroScale;
    sample.temperatureCelsius = ((static_cast<double>(tempRaw) - static_cast<double>(tempOffset)) / tempScale) + 25.0;
    error.clear();
    return true;
}

const std::string& Icm20608IioReader::devicePath() const
{
    return devicePath_;
}

bool Icm20608IioReader::readDouble(const std::string& name, double& value, std::string& error) const
{
    std::string token;
    if (!readToken(name, token, error)) {
        return false;
    }

    try {
        std::size_t parsed = 0;
        value = std::stod(token, &parsed);
        if (parsed != token.size()) {
            error = "ICM20608 IIO value has trailing data: " + name;
            return false;
        }
    } catch (const std::exception&) {
        error = "ICM20608 IIO value is not a number: " + name;
        return false;
    }

    return true;
}

bool Icm20608IioReader::readInt(const std::string& name, int& value, std::string& error) const
{
    std::string token;
    if (!readToken(name, token, error)) {
        return false;
    }

    try {
        std::size_t parsed = 0;
        value = std::stoi(token, &parsed);
        if (parsed != token.size()) {
            error = "ICM20608 IIO value has trailing data: " + name;
            return false;
        }
    } catch (const std::exception&) {
        error = "ICM20608 IIO value is not an integer: " + name;
        return false;
    }

    return true;
}

bool Icm20608IioReader::readToken(const std::string& name, std::string& value, std::string& error) const
{
    const std::filesystem::path path = std::filesystem::path(devicePath_) / name;
    std::ifstream stream(path);
    if (!stream.is_open()) {
        error = "failed to open ICM20608 IIO file: " + path.string();
        return false;
    }

    if (!(stream >> value)) {
        error = "failed to read ICM20608 IIO file: " + path.string();
        return false;
    }

    return true;
}

} // namespace outdoor::sensors
