#pragma once

#include "gnss/gnss_status.h"
#include "mcu/mcu_status.h"
#include "sensors/board_imu_status.h"

#include <cstddef>
#include <cstdint>
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
    std::size_t activeServiceCount = 0;
    std::size_t completedServiceCount = 0;
    std::string lastError;
    bool statusSocketEnabled = false;
    std::string statusSocketPath;
    bool storageEnabled = false;
    std::string storageRootPath;
    std::string storageStatusOutputPath;
    std::string storageDashboardOutputPath;
    std::string storageLogFilePath;
    std::size_t storageLogMaxBytes = 0U;
    std::size_t storageLogBackupCount = 0U;
    std::uint64_t storageLogRotationFailureCount = 0U;
    std::uint64_t storageLogWriteFailureCount = 0U;
    std::string storageLogLastError;
    bool historyEnabled = false;
    std::string historyOutputPath;
    std::string storageLastError;
    outdoor::gnss::GnssStatus gnssStatus;
    outdoor::mcu::McuStatus mcuStatus;
    outdoor::sensors::BoardImuStatus boardImuStatus;
};

const char* runtimeStateToString(RuntimeState state);

} // namespace outdoor::runtime
