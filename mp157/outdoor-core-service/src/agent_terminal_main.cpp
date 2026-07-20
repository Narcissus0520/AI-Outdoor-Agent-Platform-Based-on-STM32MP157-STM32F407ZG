#include "agent/agent_response_serializer.h"
#include "agent/local_agent_service.h"
#include "agent/mock_agent_backend.h"
#include "ipc/unix_status_client.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName
              << " --prompt text [--request-id id] [--status-socket path]"
              << " [--format json|text]\n"
              << "\nThis Stage 2 terminal uses a deterministic mock backend;"
              << " no AI model inference is configured.\n";
}

void printText(const outdoor::agent::AgentResponse& response)
{
    std::cout << "request_id: " << response.requestId << '\n'
              << "backend: " << response.backend << '\n'
              << "state: " << outdoor::agent::agentResponseStateToString(response.state) << '\n'
              << "runtime_context_available: "
              << (response.runtimeContextAvailable ? "true" : "false") << '\n'
              << "message: " << response.message << '\n';
    if (!response.errorCode.empty()) {
        std::cout << "error_code: " << response.errorCode << '\n'
                  << "error_message: " << response.errorMessage << '\n';
    }
}

} // namespace

int main(int argc, char* argv[])
{
    std::string prompt;
    std::string requestId = "cli-1";
    std::string statusSocketPath;
    std::string format = "json";

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (argument == "--prompt" || argument == "--request-id"
            || argument == "--status-socket" || argument == "--format") {
            if (index + 1 >= argc) {
                std::cerr << argument << " requires a value\n";
                return 2;
            }
            const std::string value = argv[++index];
            if (argument == "--prompt") {
                prompt = value;
            } else if (argument == "--request-id") {
                requestId = value;
            } else if (argument == "--status-socket") {
                statusSocketPath = value;
            } else {
                format = value;
            }
            continue;
        }

        std::cerr << "Unsupported argument: " << argument << '\n';
        printUsage(argv[0]);
        return 2;
    }

    if (prompt.empty()) {
        std::cerr << "--prompt is required\n";
        printUsage(argv[0]);
        return 2;
    }
    if (format != "json" && format != "text") {
        std::cerr << "--format must be json or text\n";
        return 2;
    }

    outdoor::agent::AgentRequest request;
    request.requestId = requestId;
    request.prompt = prompt;
    if (!statusSocketPath.empty()) {
        std::string error;
        if (!outdoor::ipc::UnixStatusClient::query(
                statusSocketPath,
                request.runtimeContextJson,
                error,
                2000U,
                outdoor::agent::LocalAgentService::kMaximumRuntimeContextBytes)) {
            std::cerr << "Runtime context query failed: " << error;
            if (error.empty() || error.back() != '\n') {
                std::cerr << '\n';
            }
            return 2;
        }
    }

    outdoor::agent::LocalAgentService service(
        std::make_unique<outdoor::agent::MockAgentBackend>());
    const outdoor::agent::AgentResponse response = service.process(request);
    if (format == "json") {
        std::cout << outdoor::agent::AgentResponseSerializer::toJson(response);
    } else {
        printText(response);
    }

    return response.state == outdoor::agent::AgentResponseState::Completed ? 0 : 2;
}
