#include "runtime/runtime_manager.h"

#include "log/logger.h"

#include <string>
#include <utility>

namespace outdoor::runtime {

void RuntimeManager::addService(std::unique_ptr<IService> service)
{
    services_.push_back(std::move(service));
    status_.serviceCount = services_.size();
}

bool RuntimeManager::start()
{
    status_.state = RuntimeState::Starting;
    status_.lastError.clear();
    startedServices_.clear();
    status_.startedServiceCount = 0;

    for (const auto& service : services_) {
        outdoor::log::Logger::info(std::string("Starting service: ") + service->name());
        if (!service->start()) {
            status_.state = RuntimeState::Failed;
            status_.lastError = std::string("failed to start service: ") + service->name();
            outdoor::log::Logger::error(status_.lastError);
            stop();
            return false;
        }
        startedServices_.push_back(service.get());
        status_.startedServiceCount = startedServices_.size();
    }

    status_.state = RuntimeState::Running;
    return true;
}

bool RuntimeManager::run()
{
    status_.state = RuntimeState::Running;
    status_.lastError.clear();

    for (const auto& service : services_) {
        outdoor::log::Logger::info(std::string("Running service: ") + service->name());
        if (!service->run()) {
            status_.state = RuntimeState::Failed;
            status_.lastError = std::string("service failed while running: ") + service->name();
            outdoor::log::Logger::error(status_.lastError);
            return false;
        }
    }

    return true;
}

void RuntimeManager::stop()
{
    status_.state = RuntimeState::Stopping;

    for (auto iterator = startedServices_.rbegin(); iterator != startedServices_.rend(); ++iterator) {
        outdoor::log::Logger::info(std::string("Stopping service: ") + (*iterator)->name());
        (*iterator)->stop();
    }

    startedServices_.clear();
    status_.startedServiceCount = 0;
    if (status_.lastError.empty()) {
        status_.state = RuntimeState::Stopped;
    } else {
        status_.state = RuntimeState::Failed;
    }
}

RuntimeStatus RuntimeManager::status() const
{
    return status_;
}

void RuntimeManager::setGnssStatus(const outdoor::gnss::GnssStatus& status)
{
    status_.gnssStatus = status;
}

void RuntimeManager::setMcuStatus(const outdoor::mcu::McuStatus& status)
{
    status_.mcuStatus = status;
}

void RuntimeManager::setBoardImuStatus(const outdoor::sensors::BoardImuStatus& status)
{
    status_.boardImuStatus = status;
}

} // namespace outdoor::runtime
