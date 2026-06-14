#pragma once

#include "runtime/i_service.h"
#include "runtime/runtime_status.h"

#include <memory>
#include <vector>

namespace outdoor::runtime {

class RuntimeManager {
public:
    void addService(std::unique_ptr<IService> service);
    bool start();
    bool run();
    void stop();
    RuntimeStatus status() const;
    void setMcuStatus(const outdoor::mcu::McuStatus& status);

private:
    std::vector<std::unique_ptr<IService>> services_;
    std::vector<IService*> startedServices_;
    RuntimeStatus status_;
};

} // namespace outdoor::runtime
