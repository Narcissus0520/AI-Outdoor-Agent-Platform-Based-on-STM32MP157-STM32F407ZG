#pragma once

#include "mcu/mcu_frame.h"
#include "mcu/mcu_status.h"

#include <cstdint>
#include <string>
#include <vector>

namespace outdoor::mcu {

class McuFrameParser {
public:
    bool parseFrame(const std::vector<std::uint8_t>& bytes, McuFrame& frame, std::string& error) const;
    bool applyFrame(const McuFrame& frame, McuStatus& status, std::string& error) const;

private:
    bool applyHeartbeat(const McuFrame& frame, McuStatus& status, std::string& error) const;
    bool applyMockSensor(const McuFrame& frame, McuStatus& status, std::string& error) const;
};

} // namespace outdoor::mcu
