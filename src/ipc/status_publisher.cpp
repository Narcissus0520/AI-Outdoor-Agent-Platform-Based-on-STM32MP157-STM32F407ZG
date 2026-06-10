#include "ipc/status_publisher.h"

#include <filesystem>
#include <fstream>
#include <utility>

namespace outdoor::ipc {

StatusPublisher::StatusPublisher(std::string outputPath)
    : outputPath_(std::move(outputPath)) {}

bool StatusPublisher::publish(const outdoor::runtime::RuntimeStatus& status, std::string& error) const
{
    namespace fs = std::filesystem;

    try {
        const fs::path path(outputPath_);
        const fs::path parent = path.parent_path();
        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        const fs::path tempPath = path.string() + ".tmp";
        {
            std::ofstream stream(tempPath, std::ios::trunc);
            if (!stream.is_open()) {
                error = "failed to open temporary status output path: " + tempPath.string();
                return false;
            }

            stream << "state=" << outdoor::runtime::runtimeStateToString(status.state) << '\n'
                   << "service_count=" << status.serviceCount << '\n'
                   << "started_service_count=" << status.startedServiceCount << '\n'
                   << "last_error=" << status.lastError << '\n';
        }

        if (fs::exists(path)) {
            fs::remove(path);
        }
        fs::rename(tempPath, path);
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        return false;
    }

    error.clear();
    return true;
}

const std::string& StatusPublisher::outputPath() const
{
    return outputPath_;
}

} // namespace outdoor::ipc
