#include "ipc/unix_status_client.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    constexpr const char* kDefaultSocketPath = "runtime/outdoor_core.sock";
    if (argc > 2 || (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))) {
        std::cout << "Usage: " << argv[0] << " [socket_path]\n";
        return argc > 2 ? 1 : 0;
    }

    const std::string socketPath = argc == 2 ? argv[1] : kDefaultSocketPath;
    std::string response;
    std::string error;
    if (!outdoor::ipc::UnixStatusClient::query(socketPath, response, error)) {
        std::cerr << "Status query failed: " << error;
        if (error.empty() || error.back() != '\n') {
            std::cerr << '\n';
        }
        return 1;
    }

    std::cout << response;
    return 0;
}
