#include "runtime/runtime_manager.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class FakeService final : public outdoor::runtime::IService {
public:
    FakeService(std::string serviceName,
                std::size_t pollsBeforeCompletion,
                std::vector<std::string>& trace,
                bool failOnStart = false,
                std::size_t failOnPoll = 0)
        : name_(std::move(serviceName)),
          pollsBeforeCompletion_(pollsBeforeCompletion),
          trace_(trace),
          failOnStart_(failOnStart),
          failOnPoll_(failOnPoll)
    {
    }

    const char* name() const override
    {
        return name_.c_str();
    }

    bool start() override
    {
        started_ = !failOnStart_;
        return started_;
    }

    outdoor::runtime::ServicePollResult poll() override
    {
        trace_.push_back(name_);
        ++pollCount_;
        if (failOnPoll_ > 0U && pollCount_ == failOnPoll_) {
            return outdoor::runtime::ServicePollResult::Failed;
        }
        if (pollsBeforeCompletion_ > 0U && pollCount_ >= pollsBeforeCompletion_) {
            return outdoor::runtime::ServicePollResult::Completed;
        }
        return outdoor::runtime::ServicePollResult::Running;
    }

    void stop() override
    {
        stopped_ = true;
    }

    std::size_t pollCount() const
    {
        return pollCount_;
    }

    bool stopped() const
    {
        return stopped_;
    }

private:
    std::string name_;
    std::size_t pollsBeforeCompletion_ = 0;
    std::vector<std::string>& trace_;
    bool failOnStart_ = false;
    std::size_t failOnPoll_ = 0;
    std::size_t pollCount_ = 0;
    bool started_ = false;
    bool stopped_ = false;
};

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testServicesArePolledCooperatively()
{
    std::vector<std::string> trace;
    outdoor::runtime::RuntimeManager runtime;

    auto first = std::make_unique<FakeService>("first", 3U, trace);
    auto second = std::make_unique<FakeService>("second", 2U, trace);
    auto* firstPtr = first.get();
    auto* secondPtr = second.get();
    runtime.addService(std::move(first));
    runtime.addService(std::move(second));

    std::size_t callbackCount = 0;
    runtime.setLoopCallback([&callbackCount]() {
        ++callbackCount;
    }, 0U);

    expect(runtime.start(), "runtime should start all fake services");
    expect(runtime.status().activeServiceCount == 2U, "two services should be active after start");
    expect(runtime.run(), "cooperative run should complete successfully");

    const std::vector<std::string> expectedTrace {"first", "second", "first", "second", "first"};
    expect(trace == expectedTrace, "services should be interleaved until each completes");
    expect(firstPtr->pollCount() == 3U, "first service should be polled three times");
    expect(secondPtr->pollCount() == 2U, "second service should be polled two times");
    expect(callbackCount >= 2U, "loop callback should run while services are active");
    expect(runtime.status().completedServiceCount == 2U, "both services should be completed");
    expect(runtime.status().activeServiceCount == 0U, "no service should remain active");

    runtime.stop();
    expect(firstPtr->stopped() && secondPtr->stopped(), "stop should reach all started services");
    expect(runtime.status().state == outdoor::runtime::RuntimeState::Stopped,
           "successful runtime should stop cleanly");
}

void testPollFailureIsReported()
{
    std::vector<std::string> trace;
    outdoor::runtime::RuntimeManager runtime;
    runtime.addService(std::make_unique<FakeService>("healthy", 0U, trace));
    runtime.addService(std::make_unique<FakeService>("failing", 0U, trace, false, 1U));

    expect(runtime.start(), "failure test services should start");
    expect(!runtime.run(), "poll failure should fail the runtime");
    expect(runtime.status().state == outdoor::runtime::RuntimeState::Failed,
           "poll failure should set failed state");
    expect(runtime.status().lastError.find("failing") != std::string::npos,
           "poll failure should identify the failed service");

    runtime.stop();
    expect(runtime.status().state == outdoor::runtime::RuntimeState::Failed,
           "stop should preserve the failed state");
}

void testStopPredicateEndsLongRunningServices()
{
    std::vector<std::string> trace;
    outdoor::runtime::RuntimeManager runtime;

    auto continuous = std::make_unique<FakeService>("continuous", 0U, trace);
    auto* continuousPtr = continuous.get();
    runtime.addService(std::move(continuous));

    std::size_t predicateCalls = 0;
    runtime.setStopPredicate([&predicateCalls]() {
        ++predicateCalls;
        return predicateCalls >= 4U;
    });

    expect(runtime.start(), "continuous service should start");
    expect(runtime.run(), "stop predicate should end run without failure");
    expect(continuousPtr->pollCount() == 3U, "continuous service should run until stop is requested");
    expect(runtime.status().completedServiceCount == 0U,
           "stop request should not report a continuous service as completed");

    runtime.stop();
    expect(runtime.status().state == outdoor::runtime::RuntimeState::Stopped,
           "requested stop should produce stopped state");
}

void testStartFailureRollsBackStartedServices()
{
    std::vector<std::string> trace;
    outdoor::runtime::RuntimeManager runtime;

    auto first = std::make_unique<FakeService>("first", 1U, trace);
    auto* firstPtr = first.get();
    runtime.addService(std::move(first));
    runtime.addService(std::make_unique<FakeService>("start_failure", 1U, trace, true));

    expect(!runtime.start(), "start failure should fail the runtime");
    expect(firstPtr->stopped(), "already-started services should be rolled back");
    expect(runtime.status().state == outdoor::runtime::RuntimeState::Failed,
           "start failure should preserve failed state");
}

} // namespace

int main()
{
    testServicesArePolledCooperatively();
    testPollFailureIsReported();
    testStopPredicateEndsLongRunningServices();
    testStartFailureRollsBackStartedServices();
    std::cout << "runtime_manager_tests passed\n";
    return 0;
}
