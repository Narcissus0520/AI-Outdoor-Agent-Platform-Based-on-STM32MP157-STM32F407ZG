#pragma once

#include "runtime/runtime_status.h"

#include <string>

namespace outdoor::ipc {

class StatusPublisher {
public:
    explicit StatusPublisher(std::string outputPath);

    bool publish(const outdoor::runtime::RuntimeStatus& status, std::string& error) const;
    const std::string& outputPath() const;

private:
    std::string outputPath_;
};

} // namespace outdoor::ipc
