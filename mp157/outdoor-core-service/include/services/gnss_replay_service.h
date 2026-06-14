#pragma once

#include "input/file_replay_input.h"
#include "gnss/nmea_parser.h"
#include "runtime/i_service.h"

#include <string>

namespace outdoor::services {

class GnssReplayService final : public outdoor::runtime::IService {
public:
    explicit GnssReplayService(std::string nmeaInputPath);

    const char* name() const override;
    bool start() override;
    bool run() override;
    void stop() override;

private:
    outdoor::input::FileReplayInput input_;
    outdoor::gnss::NmeaParser parser_;
    outdoor::gnss::GnssFix currentFix_;
};

} // namespace outdoor::services
