#pragma once

#include "runtime/i_service.h"
#include "sensors/board_imu_status.h"
#include "sensors/icm20608_char_reader.h"
#include "sensors/icm20608_iio_reader.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace outdoor::services {

class Icm20608Service final : public outdoor::runtime::IService {
public:
    Icm20608Service(std::string source,
                    outdoor::sensors::Icm20608CharReader charReader,
                    outdoor::sensors::Icm20608IioReader iioReader,
                    outdoor::sensors::BoardImuStatus& status,
                    std::size_t sampleCount,
                    std::uint32_t sampleIntervalMs);

    const char* name() const override;
    bool start() override;
    bool run() override;
    void stop() override;

private:
    bool selectSource(std::string& error);
    bool readSample(outdoor::sensors::Icm20608Sample& sample, std::string& error) const;

    std::string requestedSource_;
    std::string activeSource_;
    outdoor::sensors::Icm20608CharReader charReader_;
    outdoor::sensors::Icm20608IioReader reader_;
    outdoor::sensors::BoardImuStatus& status_;
    std::size_t sampleCount_;
    std::uint32_t sampleIntervalMs_;
};

} // namespace outdoor::services
