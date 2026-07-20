#pragma once

#include "gnss/gnss_status.h"
#include "mcu/mcu_status.h"
#include "runtime/i_service.h"
#include "sensors/board_imu_status.h"

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

namespace outdoor::services {

class DashboardService final : public outdoor::runtime::IService {
public:
    DashboardService(std::string outputPath,
                     std::string outputMode,
                     std::string framebufferDevice,
                     std::size_t refreshCount,
                     std::uint32_t refreshIntervalMs,
                     bool launcherEnabled,
                     std::string launcherInputDevice,
                     std::uint32_t launcherAutoStartSeconds,
                     const outdoor::gnss::GnssStatus& gnssStatus,
                     const outdoor::mcu::McuStatus& mcuStatus,
                     const outdoor::sensors::BoardImuStatus& boardImuStatus);
    ~DashboardService() override;

    const char* name() const override;
    bool start() override;
    outdoor::runtime::ServicePollResult poll() override;
    void stop() override;

private:
    struct LauncherState;

    std::string render() const;
    bool writeTextDashboard();
    bool writeFramebufferDashboard();
    outdoor::runtime::ServicePollResult pollLauncher();

    std::string outputPath_;
    std::string outputMode_;
    std::string framebufferDevice_;
    std::size_t refreshCount_;
    std::uint32_t refreshIntervalMs_;
    bool launcherEnabled_;
    std::string launcherInputDevice_;
    std::uint32_t launcherAutoStartSeconds_;
    const outdoor::gnss::GnssStatus& gnssStatus_;
    const outdoor::mcu::McuStatus& mcuStatus_;
    const outdoor::sensors::BoardImuStatus& boardImuStatus_;
    std::unique_ptr<LauncherState> launcherState_;
    std::size_t completedRefreshCount_ = 0;
    std::chrono::steady_clock::time_point nextRefreshAt_ {};
};

} // namespace outdoor::services
