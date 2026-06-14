#pragma once

#include "mcu/mcu_protocol.h"

#include <cstdint>
#include <vector>

namespace outdoor::mcu {

struct McuFrame {
    McuFrameType type = McuFrameType::Heartbeat;
    std::uint16_t sequence = 0;
    std::vector<std::uint8_t> payload;
};

} // namespace outdoor::mcu
