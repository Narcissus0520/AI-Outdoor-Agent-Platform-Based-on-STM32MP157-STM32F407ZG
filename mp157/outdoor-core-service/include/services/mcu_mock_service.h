#pragma once

#include "mcu/mcu_frame_parser.h"
#include "mcu/mcu_status.h"
#include "runtime/i_service.h"

#include <fstream>
#include <functional>
#include <string>
#include <cstdint>
#include <vector>

namespace outdoor::services {

class McuMockService final : public outdoor::runtime::IService {
public:
    McuMockService(std::string frameInputPath,
                   outdoor::mcu::McuStatus& status,
                   std::function<bool(std::string&)> statusUpdatedCallback = {});

    const char* name() const override;
    bool start() override;
    outdoor::runtime::ServicePollResult poll() override;
    void stop() override;

private:
    bool parseHexLine(const std::string& line, std::vector<std::uint8_t>& bytes, std::string& error) const;

    std::string frameInputPath_;
    std::ifstream stream_;
    std::size_t lineNumber_ = 0;
    outdoor::mcu::McuStatus& status_;
    std::function<bool(std::string&)> statusUpdatedCallback_;
    outdoor::mcu::McuFrameParser parser_;
};

} // namespace outdoor::services
