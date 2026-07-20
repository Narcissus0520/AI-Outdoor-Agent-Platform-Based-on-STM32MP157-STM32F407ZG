#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace outdoor::ipc {

class UnixStatusClient {
public:
    static bool supported();
    static bool query(const std::string& socketPath,
                      std::string& response,
                      std::string& error,
                      std::uint32_t timeoutMs = 2000U,
                      std::size_t maximumResponseBytes = 1024U * 1024U);
};

} // namespace outdoor::ipc
