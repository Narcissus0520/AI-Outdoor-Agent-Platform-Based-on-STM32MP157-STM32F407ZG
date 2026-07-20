#pragma once

#include "agent/agent_types.h"
#include "agent/i_agent_backend.h"

#include <cstddef>
#include <memory>

namespace outdoor::agent {

class LocalAgentService {
public:
    static constexpr std::uint32_t kSchemaVersion = 1U;
    static constexpr std::size_t kMaximumRequestIdBytes = 64U;
    static constexpr std::size_t kMaximumPromptBytes = 512U;
    static constexpr std::size_t kMaximumRuntimeContextBytes = 64U * 1024U;
    static constexpr std::size_t kMaximumBackendNameBytes = 32U;
    static constexpr std::size_t kMaximumResponseMessageBytes = 2U * 1024U;
    static constexpr std::size_t kMaximumErrorMessageBytes = 256U;

    explicit LocalAgentService(std::unique_ptr<IAgentBackend> backend);

    AgentResponse process(const AgentRequest& request);

private:
    std::unique_ptr<IAgentBackend> backend_;
};

} // namespace outdoor::agent
