#pragma once

#include "protocol/imu_payload.h"

#include <cstdint>
#include <string>
#include <vector>

namespace outdoor::mcu {

class ImuPayloadParser {
public:
    bool parse(const std::vector<std::uint8_t>& payload,
               outdoor::protocol::ImuSample& sample,
               std::string& error) const;
};

} // namespace outdoor::mcu
