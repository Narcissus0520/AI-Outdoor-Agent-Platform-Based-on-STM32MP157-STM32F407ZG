#include "mcu/mcu_command.h"

#include "mcu/mcu_protocol.h"

namespace outdoor::mcu {

namespace {

void appendU16Le(std::vector<std::uint8_t>& bytes, std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void appendU32Le(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

std::vector<std::uint8_t> buildFrame(McuFrameType type,
                                     std::uint16_t sequence,
                                     const std::vector<std::uint8_t>& payload)
{
    std::vector<std::uint8_t> frame;
    frame.reserve(kFrameHeaderSize + payload.size() + kFrameCrcSize);
    frame.push_back(kFrameSof0);
    frame.push_back(kFrameSof1);
    frame.push_back(kProtocolVersion);
    frame.push_back(static_cast<std::uint8_t>(type));
    appendU16Le(frame, sequence);
    appendU16Le(frame, static_cast<std::uint16_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());

    const std::uint16_t crc = crc16Modbus(frame.data() + 2, frame.size() - 2);
    appendU16Le(frame, crc);
    return frame;
}

} // namespace

bool parseMcuCommand(const std::string& value, McuCommand& command)
{
    if (value == "none") {
        command = McuCommand::None;
        return true;
    }
    if (value == "ping") {
        command = McuCommand::Ping;
        return true;
    }
    return false;
}

const char* mcuCommandToString(McuCommand command)
{
    switch (command) {
    case McuCommand::None:
        return "none";
    case McuCommand::Ping:
        return "ping";
    }
    return "unknown";
}

std::vector<std::uint8_t> buildPingCommandFrame(std::uint16_t sequence, std::uint32_t nonce)
{
    std::vector<std::uint8_t> payload;
    payload.reserve(kCommandPingPayloadSize);
    appendU32Le(payload, nonce);
    return buildFrame(McuFrameType::CommandPing, sequence, payload);
}

} // namespace outdoor::mcu
