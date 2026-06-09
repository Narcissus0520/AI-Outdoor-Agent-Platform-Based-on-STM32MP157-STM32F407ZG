#pragma once

#include "config/app_config.h"

#include <string>

namespace outdoor::config {

class ConfigLoader {
public:
    bool load(const std::string& filePath, AppConfig& config, std::string& error) const;
};

} // namespace outdoor::config
