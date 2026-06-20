#include "mcu/mcu_frame_stream_decoder.h"

#include "mcu/mcu_protocol.h"

namespace outdoor::mcu {

namespace {

std::uint16_t readU16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U);
}

} // namespace

bool McuFrameStreamDecoder::feed(const std::uint8_t* data,
                                 std::size_t size,
                                 std::vector<std::vector<std::uint8_t>>& frames,
                                 std::string& error)
{
    frames.clear();
    error.clear();

    if (data != nullptr && size > 0U) {
        buffer_.insert(buffer_.end(), data, data + size);
    }

    while (true) {
        auto sof = buffer_.end();
        for (auto it = buffer_.begin(); it != buffer_.end(); ++it) {
            const auto next = it + 1;
            if (next == buffer_.end()) {
                break;
            }
            if (*it == kFrameSof0 && *next == kFrameSof1) {
                sof = it;
                break;
            }
        }

        if (sof == buffer_.end()) {
            if (!buffer_.empty() && buffer_.back() == kFrameSof0) {
                const std::uint8_t last = buffer_.back();
                buffer_.clear();
                buffer_.push_back(last);
                return true;
            }
            buffer_.clear();
            return true;
        }
        if (sof != buffer_.begin()) {
            buffer_.erase(buffer_.begin(), sof);
        }

        if (buffer_.size() < kFrameHeaderSize) {
            return true;
        }

        if (buffer_[2] != kProtocolVersion) {
            error = "MCU stream skipped frame with unsupported protocol version";
            buffer_.erase(buffer_.begin());
            continue;
        }

        const std::uint16_t payloadSize = readU16Le(buffer_, 6U);
        if (payloadSize > kMaxPayloadSize) {
            error = "MCU stream skipped frame with oversized payload";
            buffer_.erase(buffer_.begin());
            continue;
        }

        const std::size_t frameSize = kFrameHeaderSize + payloadSize + kFrameCrcSize;
        if (buffer_.size() < frameSize) {
            return true;
        }

        frames.emplace_back(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frameSize));
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frameSize));
    }
}

void McuFrameStreamDecoder::reset()
{
    buffer_.clear();
}

} // namespace outdoor::mcu
