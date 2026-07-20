#include "ipc/unix_status_client.h"
#include "ipc/unix_status_service.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#if defined(__linux__) || defined(__unix__)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace {

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

#if defined(__linux__) || defined(__unix__)

void createStaleSocket(const std::string& path)
{
    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    std::copy(path.begin(), path.end(), address.sun_path);
    address.sun_path[path.size()] = '\0';

    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    expect(fd >= 0, "stale socket fixture should create a socket");
    expect(::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0,
           "stale socket fixture should bind");
    (void)::close(fd);
}

#endif

} // namespace

int main()
{
    expect(outdoor::ipc::UnixStatusService::responseForCommand(
               "GET_STATUS", []() { return std::string("{}"); }) == "{}\n",
           "GET_STATUS should return a newline-terminated snapshot");
    expect(outdoor::ipc::UnixStatusService::responseForCommand("PING", {}) == "PONG\n",
           "PING should not require a status provider");
    expect(outdoor::ipc::UnixStatusService::responseForCommand("UNKNOWN", {})
               == "ERR unsupported_command\n",
           "unknown commands should receive a bounded error");
    expect(outdoor::ipc::UnixStatusService::responseForCommand("GET_STATUS", {})
               == "ERR status_unavailable\n",
           "GET_STATUS without a provider should fail clearly");

#if defined(__linux__) || defined(__unix__)
    namespace fs = std::filesystem;
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path testRoot = fs::temp_directory_path()
        / ("outdoor_unix_status_test_" + std::to_string(unique));
    const fs::path socketPath = testRoot / "runtime.sock";
    fs::create_directories(testRoot);

    createStaleSocket(socketPath.string());
    outdoor::ipc::UnixStatusService service(
        socketPath.string(),
        []() { return std::string("{\"runtime\":{\"state\":\"running\"}}\n"); });
    expect(service.start(), "service should replace a stale socket: " + service.lastError());

    outdoor::ipc::UnixStatusService collision(
        socketPath.string(),
        []() { return std::string("{}\n"); });
    expect(!collision.start(), "a second service must not replace an active socket");
    expect(collision.lastError().find("already served") != std::string::npos,
           "active socket collision should have a precise error");

    std::atomic<bool> keepPolling {true};
    std::atomic<bool> pollFailed {false};
    std::thread serverThread([&]() {
        while (keepPolling.load()) {
            if (service.poll() == outdoor::runtime::ServicePollResult::Failed) {
                pollFailed.store(true);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::string response;
    std::string error;
    expect(outdoor::ipc::UnixStatusClient::query(socketPath.string(), response, error),
           "client should query the live service: " + error);
    expect(response == "{\"runtime\":{\"state\":\"running\"}}\n",
           "client should receive the exact status snapshot");

    keepPolling.store(false);
    serverThread.join();
    expect(!pollFailed.load(), "service polling should remain healthy");
    service.stop();
    expect(!fs::exists(socketPath), "service stop should unlink its socket path");
    fs::remove_all(testRoot);
#else
    expect(!outdoor::ipc::UnixStatusService::supported(),
           "non-POSIX build should report Unix sockets unsupported");
    outdoor::ipc::UnixStatusService service("runtime/test.sock", []() { return std::string("{}\n"); });
    expect(!service.start(), "non-POSIX service start should fail clearly");
    std::string response;
    std::string error;
    expect(!outdoor::ipc::UnixStatusClient::query("runtime/test.sock", response, error),
           "non-POSIX client query should fail clearly");
#endif

    std::cout << "unix_status_service_tests passed\n";
    return 0;
}
