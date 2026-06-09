#include "runtime/runtime_manager.h"

#include "log/logger.h"

#include <string>
#include <utility>

namespace outdoor::runtime {

void RuntimeManager::addService(std::unique_ptr<IService> service)
{
    services_.push_back(std::move(service));
}

bool RuntimeManager::start()
{
    startedServices_.clear();

    for (const auto& service : services_) {
        outdoor::log::Logger::info(std::string("Starting service: ") + service->name());
        if (!service->start()) {
            outdoor::log::Logger::error(std::string("Failed to start service: ") + service->name());
            stop();
            return false;
        }
        startedServices_.push_back(service.get());
    }

    return true;
}

bool RuntimeManager::run()
{
    for (const auto& service : services_) {
        outdoor::log::Logger::info(std::string("Running service: ") + service->name());
        if (!service->run()) {
            outdoor::log::Logger::error(std::string("Service failed while running: ") + service->name());
            return false;
        }
    }

    return true;
}

void RuntimeManager::stop()
{
    for (auto iterator = startedServices_.rbegin(); iterator != startedServices_.rend(); ++iterator) {
        outdoor::log::Logger::info(std::string("Stopping service: ") + (*iterator)->name());
        (*iterator)->stop();
    }

    startedServices_.clear();
}

} // namespace outdoor::runtime
