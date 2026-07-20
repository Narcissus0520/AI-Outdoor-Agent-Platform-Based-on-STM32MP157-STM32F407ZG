#include "agent/agent_response_serializer.h"
#include "agent/local_agent_service.h"
#include "agent/mock_agent_backend.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

class FixedBackend final : public outdoor::agent::IAgentBackend {
public:
    FixedBackend(std::string backendName,
                 bool success,
                 std::string message,
                 std::string error = {},
                 bool throwException = false)
        : backendName_(std::move(backendName)),
          success_(success),
          message_(std::move(message)),
          error_(std::move(error)),
          throwException_(throwException)
    {
    }

    const char* name() const override
    {
        return backendName_.c_str();
    }

    bool generate(const outdoor::agent::AgentRequest&,
                  std::string& message,
                  std::string& error) override
    {
        if (throwException_) {
            throw std::runtime_error("backend exploded");
        }
        message = message_;
        error = error_;
        return success_;
    }

private:
    std::string backendName_;
    bool success_ = false;
    std::string message_;
    std::string error_;
    bool throwException_ = false;
};

outdoor::agent::AgentRequest validRequest()
{
    outdoor::agent::AgentRequest request;
    request.requestId = "request-1";
    request.prompt = "Report current outdoor status";
    return request;
}

void expectRejected(outdoor::agent::LocalAgentService& service,
                    outdoor::agent::AgentRequest request,
                    const std::string& code)
{
    const auto response = service.process(request);
    expect(response.state == outdoor::agent::AgentResponseState::Rejected,
           code + " should reject the request");
    expect(response.errorCode == code, code + " should have a stable error code");
    expect(response.message.empty(), code + " should not contain a generated response");
}

} // namespace

int main()
{
    outdoor::agent::LocalAgentService mockService(
        std::make_unique<outdoor::agent::MockAgentBackend>());
    const auto withoutContext = mockService.process(validRequest());
    expect(withoutContext.state == outdoor::agent::AgentResponseState::Completed,
           "valid mock request should complete");
    expect(withoutContext.backend == "mock_no_inference",
           "mock backend name should explicitly describe its boundary");
    expect(!withoutContext.runtimeContextAvailable,
           "request without Runtime JSON should report no context");
    expect(withoutContext.message.find("no AI inference was executed") != std::string::npos,
           "mock response must not imply real inference");

    auto withContextRequest = validRequest();
    withContextRequest.runtimeContextJson = "{\"runtime\":{\"state\":\"running\"}}\n";
    const auto withContext = mockService.process(withContextRequest);
    expect(withContext.state == outdoor::agent::AgentResponseState::Completed,
           "valid context request should complete");
    expect(withContext.runtimeContextAvailable,
           "non-empty Runtime JSON should be marked available");

    auto invalidSchema = validRequest();
    invalidSchema.schemaVersion = 2U;
    expectRejected(mockService, invalidSchema, "unsupported_schema");

    auto invalidId = validRequest();
    invalidId.requestId = "bad id";
    expectRejected(mockService, invalidId, "invalid_request_id");
    auto longId = validRequest();
    longId.requestId.assign(outdoor::agent::LocalAgentService::kMaximumRequestIdBytes + 1U, 'a');
    expectRejected(mockService, longId, "invalid_request_id");

    auto blankPrompt = validRequest();
    blankPrompt.prompt = "   ";
    expectRejected(mockService, blankPrompt, "prompt_empty");
    auto longPrompt = validRequest();
    longPrompt.prompt.assign(outdoor::agent::LocalAgentService::kMaximumPromptBytes + 1U, 'a');
    expectRejected(mockService, longPrompt, "prompt_too_long");
    auto controlPrompt = validRequest();
    controlPrompt.prompt = "line one\nline two";
    expectRejected(mockService, controlPrompt, "prompt_contains_control");

    auto largeContext = validRequest();
    largeContext.runtimeContextJson.assign(
        outdoor::agent::LocalAgentService::kMaximumRuntimeContextBytes + 1U, 'x');
    expectRejected(mockService, largeContext, "runtime_context_too_large");

    outdoor::agent::LocalAgentService missingBackend(nullptr);
    const auto missingBackendResponse = missingBackend.process(validRequest());
    expect(missingBackendResponse.state == outdoor::agent::AgentResponseState::Failed,
           "missing backend should fail after request validation");
    expect(missingBackendResponse.errorCode == "backend_unavailable",
           "missing backend should have a stable error code");

    outdoor::agent::LocalAgentService failingBackend(
        std::make_unique<FixedBackend>("failing", false, "", std::string(400U, 'e')));
    const auto failingResponse = failingBackend.process(validRequest());
    expect(failingResponse.errorCode == "backend_failed", "backend failure should be explicit");
    expect(failingResponse.errorMessage.size()
               == outdoor::agent::LocalAgentService::kMaximumErrorMessageBytes,
           "backend errors should be bounded");

    outdoor::agent::LocalAgentService throwingBackend(
        std::make_unique<FixedBackend>("throwing", false, "", "", true));
    const auto throwingResponse = throwingBackend.process(validRequest());
    expect(throwingResponse.errorCode == "backend_exception",
           "backend exceptions should be contained");

    outdoor::agent::LocalAgentService oversizedBackend(
        std::make_unique<FixedBackend>(
            "oversized",
            true,
            std::string(outdoor::agent::LocalAgentService::kMaximumResponseMessageBytes + 1U, 'x')));
    const auto oversizedResponse = oversizedBackend.process(validRequest());
    expect(oversizedResponse.errorCode == "backend_response_too_large",
           "oversized backend response should fail closed");

    outdoor::agent::LocalAgentService escapedBackend(
        std::make_unique<FixedBackend>("fixed", true, "line one\n\"quoted\"\\tail"));
    const std::string json = outdoor::agent::AgentResponseSerializer::toJson(
        escapedBackend.process(validRequest()));
    expect(json.find("line one\\n\\\"quoted\\\"\\\\tail") != std::string::npos,
           "response JSON should escape control characters, quotes and slashes");
    expect(json.find("\"state\": \"completed\"") != std::string::npos,
           "response JSON should expose the stable state string");

    std::cout << "local_agent_service_tests passed\n";
    return 0;
}
