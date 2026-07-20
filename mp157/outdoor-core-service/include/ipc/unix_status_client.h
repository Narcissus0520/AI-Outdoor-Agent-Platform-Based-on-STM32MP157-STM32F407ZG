#pragma once

#include <cstdint>
#include <string>

namespace outdoor::ipc {

class UnixStatusClient {
public:
    static bool supported();
    static bool query(const std::string& socketPath,
                      std::string& response,
                      std::string& error,
                      std::uint32_t timeoutMs = 2000U);
};

} // namespace outdoor::ipc
