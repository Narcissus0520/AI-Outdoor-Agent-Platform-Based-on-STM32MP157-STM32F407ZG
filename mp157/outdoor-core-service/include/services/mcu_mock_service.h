#pragma once

#include "mcu/mcu_frame_parser.h"
#include "mcu/mcu_status.h"
#include "runtime/i_service.h"

#include <string>
#include <cstdint>
#include <vector>

namespace outdoor::services {

class McuMockService final : public outdoor::runtime::IService {
public:
    McuMockService(std::string frameInputPath, outdoor::mcu::McuStatus& status);

    const char* name() const override;
    bool start() override;
    bool run() override;
    void stop() override;

private:
    bool parseHexLine(const std::string& line, std::vector<std::uint8_t>& bytes, std::string& error) const;

    std::string frameInputPath_;
    outdoor::mcu::McuStatus& status_;
    outdoor::mcu::McuFrameParser parser_;
};

} // namespace outdoor::services
