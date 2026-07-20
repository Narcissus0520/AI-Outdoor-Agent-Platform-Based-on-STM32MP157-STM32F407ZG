#include "agent/local_agent_service.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

namespace outdoor::agent {

namespace {

bool validRequestId(const std::string& requestId)
{
    if (requestId.empty() || requestId.size() > LocalAgentService::kMaximumRequestIdBytes) {
        return false;
    }
    return std::all_of(requestId.begin(), requestId.end(), [](unsigned char value) {
        return (value >= 'a' && value <= 'z')
            || (value >= 'A' && value <= 'Z')
            || (value >= '0' && value <= '9')
            || value == '-' || value == '_' || value == '.';
    });
}

bool promptIsBlank(const std::string& prompt)
{
    return std::all_of(prompt.begin(), prompt.end(), [](unsigned char value) {
        return value == ' ' || value == '\t' || value == '\r'
            || value == '\n' || value == '\f' || value == '\v';
    });
}

bool promptContainsControlCharacter(const std::string& prompt)
{
    return std::any_of(prompt.begin(), prompt.end(), [](unsigned char value) {
        return value < 0x20U || value == 0x7FU;
    });
}

std::string boundedError(std::string error)
{
    if (error.empty()) {
        error = "backend returned failure without an error message";
    }
    if (error.size() > LocalAgentService::kMaximumErrorMessageBytes) {
        error.resize(LocalAgentService::kMaximumErrorMessageBytes);
    }
    return error;
}

AgentResponse makeBaseResponse(const AgentRequest& request)
{
    AgentResponse response;
    if (validRequestId(request.requestId)) {
        response.requestId = request.requestId;
    }
    return response;
}

AgentResponse reject(const AgentRequest& request,
                     std::string errorCode,
                     std::string errorMessage)
{
    AgentResponse response = makeBaseResponse(request);
    response.state = AgentResponseState::Rejected;
    response.errorCode = std::move(errorCode);
    response.errorMessage = std::move(errorMessage);
    return response;
}

AgentResponse fail(const AgentRequest& request,
                   std::string backend,
                   std::string errorCode,
                   std::string errorMessage)
{
    AgentResponse response = makeBaseResponse(request);
    response.backend = std::move(backend);
    response.state = AgentResponseState::Failed;
    response.errorCode = std::move(errorCode);
    response.errorMessage = boundedError(std::move(errorMessage));
    return response;
}

} // namespace

const char* agentResponseStateToString(AgentResponseState state)
{
    switch (state) {
    case AgentResponseState::Completed:
        return "completed";
    case AgentResponseState::Rejected:
        return "rejected";
    case AgentResponseState::Failed:
        return "failed";
    }
    return "failed";
}

LocalAgentService::LocalAgentService(std::unique_ptr<IAgentBackend> backend)
    : backend_(std::move(backend))
{
}

AgentResponse LocalAgentService::process(const AgentRequest& request)
{
    if (request.schemaVersion != kSchemaVersion) {
        return reject(request, "unsupported_schema", "only Local Agent schema version 1 is supported");
    }
    if (!validRequestId(request.requestId)) {
        return reject(request, "invalid_request_id",
                      "request_id must be 1..64 bytes using letters, digits, '.', '_' or '-'");
    }
    if (request.prompt.empty() || promptIsBlank(request.prompt)) {
        return reject(request, "prompt_empty", "prompt must not be empty");
    }
    if (request.prompt.size() > kMaximumPromptBytes) {
        return reject(request, "prompt_too_long", "prompt exceeds the 512-byte limit");
    }
    if (promptContainsControlCharacter(request.prompt)) {
        return reject(request, "prompt_contains_control", "prompt must be a single printable line");
    }
    if (request.runtimeContextJson.size() > kMaximumRuntimeContextBytes) {
        return reject(request, "runtime_context_too_large",
                      "Runtime context exceeds the 64-KiB limit");
    }
    if (!backend_) {
        return fail(request, "", "backend_unavailable", "Local Agent backend is not configured");
    }

    std::string backendName;
    try {
        const char* backendNameValue = backend_->name();
        backendName = backendNameValue == nullptr ? "" : backendNameValue;
    } catch (const std::exception& exception) {
        return fail(request, "", "backend_exception", exception.what());
    } catch (...) {
        return fail(request, "", "backend_exception", "backend name lookup threw an unknown exception");
    }
    if (backendName.empty() || backendName.size() > kMaximumBackendNameBytes) {
        return fail(request, "", "backend_contract_invalid",
                    "backend name must be 1..32 bytes");
    }

    std::string message;
    std::string error;
    try {
        if (!backend_->generate(request, message, error)) {
            return fail(request, backendName, "backend_failed", std::move(error));
        }
    } catch (const std::exception& exception) {
        return fail(request, backendName, "backend_exception", exception.what());
    } catch (...) {
        return fail(request, backendName, "backend_exception", "backend threw an unknown exception");
    }

    if (message.empty()) {
        return fail(request, backendName, "backend_response_empty",
                    "backend returned an empty response");
    }
    if (message.size() > kMaximumResponseMessageBytes) {
        return fail(request, backendName, "backend_response_too_large",
                    "backend response exceeds the 2048-byte limit");
    }

    AgentResponse response = makeBaseResponse(request);
    response.backend = backendName;
    response.state = AgentResponseState::Completed;
    response.runtimeContextAvailable = !request.runtimeContextJson.empty();
    response.message = std::move(message);
    return response;
}

} // namespace outdoor::agent
