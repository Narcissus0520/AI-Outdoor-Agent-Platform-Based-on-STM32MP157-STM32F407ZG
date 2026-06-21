#pragma once

#include "gnss/gnss_status.h"
#include "gnss/nmea_parser.h"
#include "runtime/i_service.h"

#include <cstdint>
#include <string>

namespace outdoor::services {

class GnssSerialService final : public outdoor::runtime::IService {
public:
    GnssSerialService(std::string devicePath,
                      std::uint32_t baudRate,
                      std::uint32_t captureSeconds,
                      outdoor::gnss::GnssStatus& status);
    ~GnssSerialService() override;

    const char* name() const override;
    bool start() override;
    bool run() override;
    void stop() override;

private:
    bool configurePort(std::string& error);
    void closePort();
    void consumeLine(const std::string& line);

    std::string devicePath_;
    std::uint32_t baudRate_;
    std::uint32_t captureSeconds_;
    outdoor::gnss::NmeaParser parser_;
    outdoor::gnss::GnssStatus& status_;
#ifndef _WIN32
    int fd_ = -1;
#endif
};

} // namespace outdoor::services
