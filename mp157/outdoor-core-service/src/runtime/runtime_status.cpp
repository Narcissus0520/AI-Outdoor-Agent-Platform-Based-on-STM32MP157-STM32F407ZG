#include "runtime/runtime_status.h"

namespace outdoor::runtime {

const char* runtimeStateToString(RuntimeState state)
{
    switch (state) {
    case RuntimeState::Created:
        return "created";
    case RuntimeState::Starting:
        return "starting";
    case RuntimeState::Running:
        return "running";
    case RuntimeState::Stopping:
        return "stopping";
    case RuntimeState::Stopped:
        return "stopped";
    case RuntimeState::Failed:
        return "failed";
    }

    return "unknown";
}

} // namespace outdoor::runtime
