#include "agent/agent_response_serializer.h"

#include <iomanip>
#include <sstream>

namespace outdoor::agent {

namespace {

std::string jsonEscape(const std::string& value)
{
    std::ostringstream escaped;
    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            escaped << "\\\"";
            break;
        case '\\':
            escaped << "\\\\";
            break;
        case '\b':
            escaped << "\\b";
            break;
        case '\f':
            escaped << "\\f";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            if (character < 0x20U) {
                escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned int>(character) << std::dec;
            } else {
                escaped << static_cast<char>(character);
            }
        }
    }
    return escaped.str();
}

} // namespace

std::string AgentResponseSerializer::toJson(const AgentResponse& response)
{
    std::ostringstream stream;
    stream << "{\n"
           << "  \"schema_version\": " << response.schemaVersion << ",\n"
           << "  \"request_id\": \"" << jsonEscape(response.requestId) << "\",\n"
           << "  \"backend\": \"" << jsonEscape(response.backend) << "\",\n"
           << "  \"state\": \"" << agentResponseStateToString(response.state) << "\",\n"
           << "  \"runtime_context_available\": "
           << (response.runtimeContextAvailable ? "true" : "false") << ",\n"
           << "  \"message\": \"" << jsonEscape(response.message) << "\",\n"
           << "  \"error\": {\n"
           << "    \"code\": \"" << jsonEscape(response.errorCode) << "\",\n"
           << "    \"message\": \"" << jsonEscape(response.errorMessage) << "\"\n"
           << "  }\n"
           << "}\n";
    return stream.str();
}

} // namespace outdoor::agent
