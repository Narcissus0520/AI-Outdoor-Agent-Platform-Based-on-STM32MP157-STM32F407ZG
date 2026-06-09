#pragma once

#include "runtime/i_service.h"

#include <memory>
#include <vector>

namespace outdoor::runtime {

class RuntimeManager {
public:
    void addService(std::unique_ptr<IService> service);
    bool start();
    bool run();
    void stop();

private:
    std::vector<std::unique_ptr<IService>> services_;
    std::vector<IService*> startedServices_;
};

} // namespace outdoor::runtime
