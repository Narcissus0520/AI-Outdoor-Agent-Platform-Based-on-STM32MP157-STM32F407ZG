#pragma once

#include "gnss/gnss_status.h"
#include "mcu/mcu_status.h"
#include "runtime/i_service.h"
#include "sensors/board_imu_status.h"

#include <cstdint>
#include <cstddef>
#include <string>

namespace outdoor::services {

class DashboardService final : public outdoor::runtime::IService {
public:
    DashboardService(std::string outputPath,
                     std::string outputMode,
                     std::string framebufferDevice,
                     std::size_t refreshCount,
                     std::uint32_t refreshIntervalMs,
                     const outdoor::gnss::GnssStatus& gnssStatus,
                     const outdoor::mcu::McuStatus& mcuStatus,
                     const outdoor::sensors::BoardImuStatus& boardImuStatus);

    const char* name() const override;
    bool start() override;
    bool run() override;
    void stop() override;

private:
    std::string render() const;
    bool writeTextDashboard();
    bool writeFramebufferDashboard();

    std::string outputPath_;
    std::string outputMode_;
    std::string framebufferDevice_;
    std::size_t refreshCount_;
    std::uint32_t refreshIntervalMs_;
    const outdoor::gnss::GnssStatus& gnssStatus_;
    const outdoor::mcu::McuStatus& mcuStatus_;
    const outdoor::sensors::BoardImuStatus& boardImuStatus_;
};

} // namespace outdoor::services
