#pragma once

#include "mcu/mcu_command.h"
#include "mcu/mcu_frame_parser.h"
#include "mcu/mcu_frame_stream_decoder.h"
#include "mcu/mcu_status.h"
#include "runtime/i_service.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace outdoor::services {

class McuSerialService final : public outdoor::runtime::IService {
public:
    McuSerialService(std::string devicePath,
                     std::uint32_t baudRate,
                     std::uint32_t captureSeconds,
                     outdoor::mcu::McuCommand command,
                     outdoor::mcu::McuStatus& status,
                     std::function<bool(std::string&)> statusUpdatedCallback = {});
    ~McuSerialService() override;

    const char* name() const override;
    bool start() override;
    outdoor::runtime::ServicePollResult poll() override;
    void stop() override;

private:
    bool configurePort(std::string& error);
    bool prepareConfiguredCommand(std::string& error);
    outdoor::runtime::ServicePollResult pollConfiguredCommand();
    void closePort();

    std::string devicePath_;
    std::uint32_t baudRate_ = 115200;
    std::uint32_t captureSeconds_ = 5;
    outdoor::mcu::McuCommand command_ = outdoor::mcu::McuCommand::None;
    outdoor::mcu::McuStatus& status_;
    std::function<bool(std::string&)> statusUpdatedCallback_;
    outdoor::mcu::McuFrameParser parser_;
    outdoor::mcu::McuFrameStreamDecoder streamDecoder_;
    std::chrono::steady_clock::time_point deadline_ {};
    bool deadlineEnabled_ = false;
    std::vector<std::uint8_t> commandFrame_;
    std::size_t commandBytesWritten_ = 0;
    bool commandSent_ = false;
    int fd_ = -1;
};

} // namespace outdoor::services
