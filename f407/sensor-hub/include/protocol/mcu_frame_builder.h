#pragma once

#include "protocol/imu_payload.h"

#include <cstdint>
#include <vector>

namespace outdoor::f407::protocol {

class McuFrameBuilder {
public:
    std::vector<std::uint8_t> buildImuFrame(std::uint16_t sequence,
                                            const outdoor::protocol::ImuSample& sample) const;
};

} // namespace outdoor::f407::protocol
