#pragma once

#include "protocol/imu_payload.h"

#include <string>

namespace outdoor::f407::drivers {

class Icm42688Driver {
public:
    bool readSample(outdoor::protocol::ImuSample& sample, std::string& error);
};

} // namespace outdoor::f407::drivers
