#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace outdoor::mcu {

class McuFrameStreamDecoder {
public:
    bool feed(const std::uint8_t* data,
              std::size_t size,
              std::vector<std::vector<std::uint8_t>>& frames,
              std::string& error);

    void reset();

private:
    std::vector<std::uint8_t> buffer_;
};

} // namespace outdoor::mcu
