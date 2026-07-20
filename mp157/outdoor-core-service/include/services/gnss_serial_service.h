#pragma once

#include "gnss/gnss_status.h"
#include "gnss/nmea_parser.h"
#include "runtime/i_service.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace outdoor::services {

class GnssSerialService final : public outdoor::runtime::IService {
public:
    GnssSerialService(std::string devicePath,
                      std::uint32_t baudRate,
                      std::uint32_t captureSeconds,
                      outdoor::gnss::GnssStatus& status,
                      std::function<bool(std::string&)> statusUpdatedCallback = {});
    ~GnssSerialService() override;

    const char* name() const override;
    bool start() override;
    outdoor::runtime::ServicePollResult poll() override;
    void stop() override;

private:
    bool configurePort(std::string& error);
    void closePort();
    bool consumeLine(const std::string& line);

    std::string devicePath_;
    std::uint32_t baudRate_;
    std::uint32_t captureSeconds_;
    outdoor::gnss::NmeaParser parser_;
    outdoor::gnss::GnssStatus& status_;
    std::function<bool(std::string&)> statusUpdatedCallback_;
    std::chrono::steady_clock::time_point deadline_ {};
    std::string lineBuffer_;
    bool deadlineEnabled_ = false;
#ifndef _WIN32
    int fd_ = -1;
#endif
};

} // namespace outdoor::services
