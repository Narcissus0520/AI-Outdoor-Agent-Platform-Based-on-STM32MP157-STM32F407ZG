#pragma once

#include "agent/i_agent_backend.h"

namespace outdoor::agent {

class MockAgentBackend final : public IAgentBackend {
public:
    const char* name() const override;
    bool generate(const AgentRequest& request,
                  std::string& message,
                  std::string& error) override;
};

} // namespace outdoor::agent
