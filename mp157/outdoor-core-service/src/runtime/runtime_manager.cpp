#include "runtime/runtime_manager.h"

#include "log/logger.h"

#include <chrono>
#include <exception>
#include <string>
#include <thread>
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
    completedServices_.assign(services_.size(), false);
    status_.startedServiceCount = 0;
    status_.activeServiceCount = 0;
    status_.completedServiceCount = 0;

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
        status_.activeServiceCount = startedServices_.size();
    }

    status_.state = RuntimeState::Running;
    return true;
}

bool RuntimeManager::run()
{
    status_.state = RuntimeState::Running;
    status_.lastError.clear();

    for (const auto& service : services_) {
        outdoor::log::Logger::info(std::string("Scheduling service: ") + service->name());
    }

    auto nextLoopCallback = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(loopCallbackIntervalMs_);

    while (status_.completedServiceCount < startedServices_.size()) {
        if (stopPredicate_ && stopPredicate_()) {
            outdoor::log::Logger::info("Runtime stop requested");
            return true;
        }

        bool anyServiceRunning = false;
        for (std::size_t index = 0; index < startedServices_.size(); ++index) {
            if (completedServices_[index]) {
                continue;
            }

            ServicePollResult result = ServicePollResult::Failed;
            try {
                result = startedServices_[index]->poll();
            } catch (const std::exception& exception) {
                status_.lastError = std::string("service threw while polling: ")
                    + startedServices_[index]->name() + ": " + exception.what();
                status_.state = RuntimeState::Failed;
                outdoor::log::Logger::error(status_.lastError);
                return false;
            } catch (...) {
                status_.lastError = std::string("service threw while polling: ")
                    + startedServices_[index]->name();
                status_.state = RuntimeState::Failed;
                outdoor::log::Logger::error(status_.lastError);
                return false;
            }

            if (result == ServicePollResult::Failed) {
                status_.state = RuntimeState::Failed;
                status_.lastError = std::string("service failed while polling: ")
                    + startedServices_[index]->name();
                outdoor::log::Logger::error(status_.lastError);
                return false;
            }

            if (result == ServicePollResult::Completed) {
                completedServices_[index] = true;
                ++status_.completedServiceCount;
                status_.activeServiceCount = startedServices_.size() - status_.completedServiceCount;
                outdoor::log::Logger::info(std::string("Service completed: ")
                                           + startedServices_[index]->name());
                continue;
            }

            anyServiceRunning = true;
        }

        const auto now = std::chrono::steady_clock::now();
        if (loopCallback_ && now >= nextLoopCallback) {
            try {
                loopCallback_();
            } catch (const std::exception& exception) {
                status_.state = RuntimeState::Failed;
                status_.lastError = std::string("runtime loop callback failed: ") + exception.what();
                outdoor::log::Logger::error(status_.lastError);
                return false;
            } catch (...) {
                status_.state = RuntimeState::Failed;
                status_.lastError = "runtime loop callback failed";
                outdoor::log::Logger::error(status_.lastError);
                return false;
            }
            nextLoopCallback = now + std::chrono::milliseconds(loopCallbackIntervalMs_);
        }

        if (status_.completedServiceCount >= startedServices_.size()) {
            break;
        }

        if (anyServiceRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
    status_.activeServiceCount = 0;
    if (status_.lastError.empty()) {
        status_.state = RuntimeState::Stopped;
    } else {
        status_.state = RuntimeState::Failed;
    }
}

void RuntimeManager::setLoopCallback(std::function<void()> callback, std::uint32_t intervalMs)
{
    loopCallback_ = std::move(callback);
    loopCallbackIntervalMs_ = intervalMs;
}

void RuntimeManager::setStopPredicate(std::function<bool()> predicate)
{
    stopPredicate_ = std::move(predicate);
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
