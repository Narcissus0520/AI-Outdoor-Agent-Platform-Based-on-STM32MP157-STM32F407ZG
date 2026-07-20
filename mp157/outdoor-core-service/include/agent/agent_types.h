#pragma once

#include <cstdint>
#include <string>

namespace outdoor::agent {

enum class AgentResponseState {
    Completed,
    Rejected,
    Failed,
};

const char* agentResponseStateToString(AgentResponseState state);

struct AgentRequest {
    std::uint32_t schemaVersion = 1U;
    std::string requestId;
    std::string prompt;
    std::string runtimeContextJson;
};

struct AgentResponse {
    std::uint32_t schemaVersion = 1U;
    std::string requestId;
    std::string backend;
    AgentResponseState state = AgentResponseState::Failed;
    bool runtimeContextAvailable = false;
    std::string message;
    std::string errorCode;
    std::string errorMessage;
};

} // namespace outdoor::agent
