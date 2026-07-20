#pragma once

#include "runtime/i_service.h"
#include "runtime/runtime_status.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace outdoor::runtime {

class RuntimeManager {
public:
    void addService(std::unique_ptr<IService> service);
    bool start();
    bool run();
    void stop();
    void setLoopCallback(std::function<void()> callback, std::uint32_t intervalMs);
    void setStopPredicate(std::function<bool()> predicate);
    RuntimeStatus status() const;
    void setGnssStatus(const outdoor::gnss::GnssStatus& status);
    void setMcuStatus(const outdoor::mcu::McuStatus& status);
    void setBoardImuStatus(const outdoor::sensors::BoardImuStatus& status);

private:
    std::vector<std::unique_ptr<IService>> services_;
    std::vector<IService*> startedServices_;
    std::vector<bool> completedServices_;
    std::function<void()> loopCallback_;
    std::function<bool()> stopPredicate_;
    std::uint32_t loopCallbackIntervalMs_ = 1000U;
    RuntimeStatus status_;
};

} // namespace outdoor::runtime
