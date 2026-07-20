#include "ipc/unix_status_service.h"

#include "log/logger.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <exception>
#include <filesystem>
#include <utility>

#if defined(__linux__) || defined(__unix__)
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace outdoor::ipc {

namespace {

constexpr std::size_t kMaximumAcceptsPerPoll = 8U;
constexpr std::size_t kMaximumRequestBytes = 64U;
constexpr auto kClientIdleTimeout = std::chrono::seconds(5);

#if defined(__linux__) || defined(__unix__)

std::string systemError(const std::string& operation, int errorNumber)
{
    return operation + ": " + std::strerror(errorNumber);
}

bool configureDescriptor(int fd, std::string& error)
{
    const int statusFlags = ::fcntl(fd, F_GETFL, 0);
    if (statusFlags < 0 || ::fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) < 0) {
        error = systemError("failed to enable non-blocking socket mode", errno);
        return false;
    }

    const int descriptorFlags = ::fcntl(fd, F_GETFD, 0);
    if (descriptorFlags < 0 || ::fcntl(fd, F_SETFD, descriptorFlags | FD_CLOEXEC) < 0) {
        error = systemError("failed to enable close-on-exec", errno);
        return false;
    }
    return true;
}

bool populateAddress(const std::string& socketPath, sockaddr_un& address, std::string& error)
{
    if (socketPath.empty()) {
        error = "status socket path must not be empty";
        return false;
    }
    if (socketPath.size() >= sizeof(address.sun_path)) {
        error = "status socket path is too long for sockaddr_un: " + socketPath;
        return false;
    }

    address = {};
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, socketPath.c_str(), socketPath.size() + 1U);
    return true;
}

bool removeStaleSocket(const std::string& socketPath, const sockaddr_un& address, std::string& error)
{
    struct stat metadata {};
    if (::lstat(socketPath.c_str(), &metadata) != 0) {
        if (errno == ENOENT) {
            return true;
        }
        error = systemError("failed to inspect existing status socket path", errno);
        return false;
    }

    if (!S_ISSOCK(metadata.st_mode)) {
        error = "status socket path exists and is not a socket: " + socketPath;
        return false;
    }

    const int probeFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (probeFd < 0) {
        error = systemError("failed to create status socket probe", errno);
        return false;
    }

    const int connectResult = ::connect(
        probeFd,
        reinterpret_cast<const sockaddr*>(&address),
        static_cast<socklen_t>(sizeof(address)));
    const int connectError = errno;
    (void)::close(probeFd);

    if (connectResult == 0) {
        error = "status socket is already served by another process: " + socketPath;
        return false;
    }
    if (connectError != ECONNREFUSED && connectError != ENOENT) {
        error = systemError("failed to probe existing status socket", connectError);
        return false;
    }
    if (connectError == ENOENT) {
        return true;
    }
    if (::unlink(socketPath.c_str()) != 0 && errno != ENOENT) {
        error = systemError("failed to remove stale status socket", errno);
        return false;
    }
    return true;
}

bool wouldBlock(int errorNumber)
{
    return errorNumber == EAGAIN || errorNumber == EWOULDBLOCK;
}

#endif

} // namespace

UnixStatusService::UnixStatusService(std::string socketPath, StatusProvider statusProvider)
    : socketPath_(std::move(socketPath)),
      statusProvider_(std::move(statusProvider))
{
}

UnixStatusService::~UnixStatusService()
{
    stop();
}

bool UnixStatusService::supported()
{
#if defined(__linux__) || defined(__unix__)
    return true;
#else
    return false;
#endif
}

std::string UnixStatusService::responseForCommand(const std::string& command,
                                                  const StatusProvider& statusProvider)
{
    if (command == "PING") {
        return "PONG\n";
    }
    if (command != "GET_STATUS") {
        return "ERR unsupported_command\n";
    }
    if (!statusProvider) {
        return "ERR status_unavailable\n";
    }

    try {
        std::string response = statusProvider();
        if (response.empty() || response.back() != '\n') {
            response.push_back('\n');
        }
        return response;
    } catch (const std::exception& exception) {
        outdoor::log::Logger::warn(
            std::string("Status query provider failed: ") + exception.what());
        return "ERR status_unavailable\n";
    } catch (...) {
        outdoor::log::Logger::warn("Status query provider failed");
        return "ERR status_unavailable\n";
    }
}

const char* UnixStatusService::name() const
{
    return "unix_status";
}

bool UnixStatusService::fail(const std::string& message)
{
    lastError_ = message;
    outdoor::log::Logger::error("Unix status service: " + message);
    return false;
}

bool UnixStatusService::start()
{
    if (started_) {
        return true;
    }
    lastError_.clear();

#if defined(__linux__) || defined(__unix__)
    if (!statusProvider_) {
        return fail("status provider is not configured");
    }

    sockaddr_un address {};
    std::string error;
    if (!populateAddress(socketPath_, address, error)) {
        return fail(error);
    }

    try {
        const std::filesystem::path parent = std::filesystem::path(socketPath_).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
    } catch (const std::filesystem::filesystem_error& exception) {
        return fail(std::string("failed to create status socket directory: ") + exception.what());
    }

    if (!removeStaleSocket(socketPath_, address, error)) {
        return fail(error);
    }

    listenerFd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenerFd_ < 0) {
        return fail(systemError("failed to create status socket", errno));
    }
    if (!configureDescriptor(listenerFd_, error)) {
        stop();
        return fail(error);
    }
    if (::bind(listenerFd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        const std::string message = systemError("failed to bind status socket", errno);
        stop();
        return fail(message);
    }
    ownsSocketPath_ = true;
    if (::chmod(socketPath_.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {
        const std::string message = systemError("failed to set status socket permissions", errno);
        stop();
        return fail(message);
    }
    if (::listen(listenerFd_, static_cast<int>(kMaximumClients)) != 0) {
        const std::string message = systemError("failed to listen on status socket", errno);
        stop();
        return fail(message);
    }

    started_ = true;
    outdoor::log::Logger::info("Unix status socket listening: " + socketPath_);
    return true;
#else
    return fail("Unix domain sockets are not supported on this platform");
#endif
}

bool UnixStatusService::acceptPendingClients()
{
#if defined(__linux__) || defined(__unix__)
    for (std::size_t accepted = 0U; accepted < kMaximumAcceptsPerPoll; ++accepted) {
        const int clientFd = ::accept(listenerFd_, nullptr, nullptr);
        if (clientFd < 0) {
            if (wouldBlock(errno)) {
                return true;
            }
            if (errno == EINTR) {
                continue;
            }
            return fail(systemError("failed to accept status client", errno));
        }

        const auto slot = std::find_if(clients_.begin(), clients_.end(), [](const Client& client) {
            return client.fd < 0;
        });
        if (slot == clients_.end()) {
            (void)::close(clientFd);
            continue;
        }

        std::string error;
        if (!configureDescriptor(clientFd, error)) {
            (void)::close(clientFd);
            return fail(error);
        }

        slot->fd = clientFd;
        slot->request.clear();
        slot->response.clear();
        slot->responseOffset = 0U;
        slot->deadline = std::chrono::steady_clock::now() + kClientIdleTimeout;
    }
    return true;
#else
    return false;
#endif
}

bool UnixStatusService::serviceClient(Client& client)
{
#if defined(__linux__) || defined(__unix__)
    if (client.response.empty()) {
        char buffer[128] {};
        const ssize_t received = ::recv(client.fd, buffer, sizeof(buffer), 0);
        if (received == 0) {
            return false;
        }
        if (received < 0) {
            if (errno == EINTR || wouldBlock(errno)) {
                return true;
            }
            return false;
        }

        client.deadline = std::chrono::steady_clock::now() + kClientIdleTimeout;
        client.request.append(buffer, static_cast<std::size_t>(received));
        const std::size_t lineEnd = client.request.find('\n');
        if (lineEnd == std::string::npos && client.request.size() <= kMaximumRequestBytes) {
            return true;
        }

        if (lineEnd == std::string::npos || lineEnd > kMaximumRequestBytes) {
            client.response = "ERR request_too_long\n";
        } else {
            std::string command = client.request.substr(0U, lineEnd);
            if (!command.empty() && command.back() == '\r') {
                command.pop_back();
            }

            client.response = responseForCommand(command, statusProvider_);
        }
    }

    const char* data = client.response.data() + client.responseOffset;
    const std::size_t remaining = client.response.size() - client.responseOffset;
    const ssize_t sent = ::send(client.fd, data, remaining, MSG_NOSIGNAL);
    if (sent > 0) {
        client.responseOffset += static_cast<std::size_t>(sent);
        client.deadline = std::chrono::steady_clock::now() + kClientIdleTimeout;
        return client.responseOffset < client.response.size();
    }
    if (sent < 0 && (errno == EINTR || wouldBlock(errno))) {
        return true;
    }
    return false;
#else
    (void)client;
    return false;
#endif
}

outdoor::runtime::ServicePollResult UnixStatusService::poll()
{
    if (!started_) {
        (void)fail("poll requested before start");
        return outdoor::runtime::ServicePollResult::Failed;
    }
    if (!acceptPendingClients()) {
        return outdoor::runtime::ServicePollResult::Failed;
    }

    const auto now = std::chrono::steady_clock::now();
    for (auto& client : clients_) {
        if (client.fd < 0) {
            continue;
        }
        const bool keepClient = now < client.deadline && serviceClient(client);
        if (!keepClient) {
            closeClient(client);
        }
    }
    return outdoor::runtime::ServicePollResult::Running;
}

void UnixStatusService::closeClient(Client& client)
{
#if defined(__linux__) || defined(__unix__)
    if (client.fd >= 0) {
        (void)::close(client.fd);
    }
#endif
    client.fd = -1;
    client.request.clear();
    client.response.clear();
    client.responseOffset = 0U;
}

void UnixStatusService::stop()
{
    for (auto& client : clients_) {
        closeClient(client);
    }
#if defined(__linux__) || defined(__unix__)
    if (listenerFd_ >= 0) {
        (void)::close(listenerFd_);
    }
    if (ownsSocketPath_) {
        (void)::unlink(socketPath_.c_str());
    }
#endif

    listenerFd_ = -1;
    ownsSocketPath_ = false;
    started_ = false;
}

const std::string& UnixStatusService::socketPath() const
{
    return socketPath_;
}

const std::string& UnixStatusService::lastError() const
{
    return lastError_;
}

} // namespace outdoor::ipc
