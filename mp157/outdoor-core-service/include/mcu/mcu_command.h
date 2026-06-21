#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace outdoor::mcu {

enum class McuCommand {
    None,
    Ping,
};

constexpr std::uint32_t kDefaultPingNonce = 0x50494E47U;

bool parseMcuCommand(const std::string& value, McuCommand& command);
const char* mcuCommandToString(McuCommand command);
std::vector<std::uint8_t> buildPingCommandFrame(std::uint16_t sequence, std::uint32_t nonce);

} // namespace outdoor::mcu
