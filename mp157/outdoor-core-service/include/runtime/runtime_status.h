#pragma once

#include "gnss/gnss_status.h"
#include "mcu/mcu_status.h"
#include "sensors/board_imu_status.h"

#include <cstddef>
#include <string>

namespace outdoor::runtime {

enum class RuntimeState {
    Created,
    Starting,
    Running,
    Stopping,
    Stopped,
    Failed,
};

struct RuntimeStatus {
    RuntimeState state = RuntimeState::Created;
    std::size_t serviceCount = 0;
    std::size_t startedServiceCount = 0;
    std::string lastError;
    bool storageEnabled = false;
    std::string storageRootPath;
    std::string storageStatusOutputPath;
    std::string storageDashboardOutputPath;
    std::string storageLogFilePath;
    std::string storageLastError;
    outdoor::gnss::GnssStatus gnssStatus;
    outdoor::mcu::McuStatus mcuStatus;
    outdoor::sensors::BoardImuStatus boardImuStatus;
};

const char* runtimeStateToString(RuntimeState state);

} // namespace outdoor::runtime
