#pragma once

#include "mcu/mcu_status.h"

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
    outdoor::mcu::McuStatus mcuStatus;
};

const char* runtimeStateToString(RuntimeState state);

} // namespace outdoor::runtime
