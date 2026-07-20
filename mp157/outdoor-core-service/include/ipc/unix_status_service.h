#pragma once

#include "runtime/i_service.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

namespace outdoor::ipc {

class UnixStatusService final : public outdoor::runtime::IService {
public:
    using StatusProvider = std::function<std::string()>;

    UnixStatusService(std::string socketPath, StatusProvider statusProvider);
    ~UnixStatusService() override;

    UnixStatusService(const UnixStatusService&) = delete;
    UnixStatusService& operator=(const UnixStatusService&) = delete;

    static bool supported();
    static std::string responseForCommand(const std::string& command,
                                          const StatusProvider& statusProvider);

    const char* name() const override;
    bool start() override;
    outdoor::runtime::ServicePollResult poll() override;
    void stop() override;

    const std::string& socketPath() const;
    const std::string& lastError() const;

private:
    static constexpr std::size_t kMaximumClients = 4U;

    struct Client {
        int fd = -1;
        std::string request;
        std::string response;
        std::size_t responseOffset = 0U;
        std::chrono::steady_clock::time_point deadline;
    };

    bool fail(const std::string& message);
    bool acceptPendingClients();
    bool serviceClient(Client& client);
    void closeClient(Client& client);

    std::string socketPath_;
    StatusProvider statusProvider_;
    int listenerFd_ = -1;
    bool started_ = false;
    bool ownsSocketPath_ = false;
    std::string lastError_;
    std::array<Client, kMaximumClients> clients_ {};
};

} // namespace outdoor::ipc
