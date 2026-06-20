#pragma once

#include "mcu/mcu_frame_parser.h"
#include "mcu/mcu_frame_stream_decoder.h"
#include "mcu/mcu_status.h"
#include "runtime/i_service.h"

#include <cstdint>
#include <string>

namespace outdoor::services {

class McuSerialService final : public outdoor::runtime::IService {
public:
    McuSerialService(std::string devicePath,
                     std::uint32_t baudRate,
                     std::uint32_t captureSeconds,
                     outdoor::mcu::McuStatus& status);
    ~McuSerialService() override;

    const char* name() const override;
    bool start() override;
    bool run() override;
    void stop() override;

private:
    bool configurePort(std::string& error);
    void closePort();

    std::string devicePath_;
    std::uint32_t baudRate_ = 115200;
    std::uint32_t captureSeconds_ = 5;
    outdoor::mcu::McuStatus& status_;
    outdoor::mcu::McuFrameParser parser_;
    outdoor::mcu::McuFrameStreamDecoder streamDecoder_;
    int fd_ = -1;
};

} // namespace outdoor::services
