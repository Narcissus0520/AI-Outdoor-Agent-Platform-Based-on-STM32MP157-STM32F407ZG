#include "agent/mock_agent_backend.h"

namespace outdoor::agent {

const char* MockAgentBackend::name() const
{
    return "mock_no_inference";
}

bool MockAgentBackend::generate(const AgentRequest& request,
                                std::string& message,
                                std::string& error)
{
    error.clear();
    if (request.runtimeContextJson.empty()) {
        message = "Mock backend only: request accepted without Runtime context; no AI inference was executed.";
    } else {
        message = "Mock backend only: request accepted with Runtime context; no AI inference was executed.";
    }
    return true;
}

} // namespace outdoor::agent
