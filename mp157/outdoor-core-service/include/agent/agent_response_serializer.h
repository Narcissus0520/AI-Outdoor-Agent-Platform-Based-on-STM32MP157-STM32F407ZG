#pragma once

#include "agent/agent_types.h"

#include <string>

namespace outdoor::agent {

class AgentResponseSerializer {
public:
    static std::string toJson(const AgentResponse& response);
};

} // namespace outdoor::agent
