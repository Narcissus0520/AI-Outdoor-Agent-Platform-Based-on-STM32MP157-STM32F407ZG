#pragma once

#include "agent/agent_types.h"

#include <string>

namespace outdoor::agent {

class IAgentBackend {
public:
    virtual ~IAgentBackend() = default;

    virtual const char* name() const = 0;
    virtual bool generate(const AgentRequest& request,
                          std::string& message,
                          std::string& error) = 0;
};

} // namespace outdoor::agent
