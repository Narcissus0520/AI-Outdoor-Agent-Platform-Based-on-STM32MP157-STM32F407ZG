#include "ipc/unix_status_client.h"

#include <cerrno>
#include <cstring>

#if defined(__linux__) || defined(__unix__)
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace outdoor::ipc {

namespace {

#if defined(__linux__) || defined(__unix__)

std::string systemError(const std::string& operation, int errorNumber)
{
    return operation + ": " + std::strerror(errorNumber);
}

bool waitForEvent(int fd, short events, std::uint32_t timeoutMs, std::string& error)
{
    pollfd descriptor {};
    descriptor.fd = fd;
    descriptor.events = events;
    const int result = ::poll(&descriptor, 1, static_cast<int>(timeoutMs));
    if (result == 0) {
        error = "status socket query timed out";
        return false;
    }
    if (result < 0) {
        error = systemError("failed while waiting for status socket", errno);
        return false;
    }
    if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0) {
        error = "status socket reported an I/O error";
        return false;
    }
    return true;
}

#endif

} // namespace

bool UnixStatusClient::supported()
{
#if defined(__linux__) || defined(__unix__)
    return true;
#else
    return false;
#endif
}

bool UnixStatusClient::query(const std::string& socketPath,
                             std::string& response,
                             std::string& error,
                             std::uint32_t timeoutMs,
                             std::size_t maximumResponseBytes)
{
    response.clear();
    error.clear();

#if defined(__linux__) || defined(__unix__)
    sockaddr_un address {};
    if (socketPath.empty()) {
        error = "status socket path must not be empty";
        return false;
    }
    if (maximumResponseBytes == 0U) {
        error = "status response limit must be greater than zero";
        return false;
    }
    if (socketPath.size() >= sizeof(address.sun_path)) {
        error = "status socket path is too long for sockaddr_un: " + socketPath;
        return false;
    }
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, socketPath.c_str(), socketPath.size() + 1U);

    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        error = systemError("failed to create status socket client", errno);
        return false;
    }

    const auto closeSocket = [&]() {
        (void)::close(fd);
    };
    if (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        error = systemError("failed to connect to status socket", errno);
        closeSocket();
        return false;
    }

    constexpr char request[] = "GET_STATUS\n";
    std::size_t sentTotal = 0U;
    while (sentTotal < sizeof(request) - 1U) {
        if (!waitForEvent(fd, POLLOUT, timeoutMs, error)) {
            closeSocket();
            return false;
        }
        const ssize_t sent = ::send(
            fd, request + sentTotal, sizeof(request) - 1U - sentTotal, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            error = systemError("failed to send status query", errno);
            closeSocket();
            return false;
        }
        sentTotal += static_cast<std::size_t>(sent);
    }
    (void)::shutdown(fd, SHUT_WR);

    char buffer[4096] {};
    for (;;) {
        if (!waitForEvent(fd, POLLIN, timeoutMs, error)) {
            closeSocket();
            return false;
        }
        const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);
        if (received == 0) {
            break;
        }
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            error = systemError("failed to receive status response", errno);
            closeSocket();
            return false;
        }
        response.append(buffer, static_cast<std::size_t>(received));
        if (response.size() > maximumResponseBytes) {
            error = "status response exceeds configured limit";
            closeSocket();
            return false;
        }
    }
    closeSocket();

    if (response.empty()) {
        error = "status socket returned an empty response";
        return false;
    }
    if (response.rfind("ERR ", 0U) == 0U) {
        error = response;
        return false;
    }
    return true;
#else
    (void)socketPath;
    (void)timeoutMs;
    (void)maximumResponseBytes;
    error = "Unix domain sockets are not supported on this platform";
    return false;
#endif
}

} // namespace outdoor::ipc
