#include "drivers/icm42688_driver.h"

namespace outdoor::f407::drivers {

bool Icm42688Driver::readSample(outdoor::protocol::ImuSample& sample, std::string& error)
{
    sample = {};
    error = "ICM42688 driver is not supported until real hardware is available";
    return false;
}

} // namespace outdoor::f407::drivers
