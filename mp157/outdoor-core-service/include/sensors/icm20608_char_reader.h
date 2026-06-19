#pragma once

#include "sensors/icm20608_iio_reader.h"

#include <string>

namespace outdoor::sensors {

class Icm20608CharReader {
public:
    explicit Icm20608CharReader(std::string devicePath);

    bool read(Icm20608Sample& sample, std::string& error) const;
    const std::string& devicePath() const;

private:
    std::string devicePath_;
};

} // namespace outdoor::sensors
