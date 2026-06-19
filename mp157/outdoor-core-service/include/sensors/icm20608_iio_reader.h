#pragma once

#include <string>

namespace outdoor::sensors {

struct Icm20608Sample {
    double accelXG = 0.0;
    double accelYG = 0.0;
    double accelZG = 0.0;
    double gyroXDps = 0.0;
    double gyroYDps = 0.0;
    double gyroZDps = 0.0;
    double temperatureCelsius = 0.0;
};

class Icm20608IioReader {
public:
    explicit Icm20608IioReader(std::string devicePath);

    bool read(Icm20608Sample& sample, std::string& error) const;
    const std::string& devicePath() const;

private:
    bool readDouble(const std::string& name, double& value, std::string& error) const;
    bool readInt(const std::string& name, int& value, std::string& error) const;
    bool readToken(const std::string& name, std::string& value, std::string& error) const;

    std::string devicePath_;
};

} // namespace outdoor::sensors
