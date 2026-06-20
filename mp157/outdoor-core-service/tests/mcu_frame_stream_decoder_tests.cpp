#include "mcu/mcu_frame_parser.h"
#include "mcu/mcu_frame_stream_decoder.h"
#include "mcu/mcu_protocol.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

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

std::vector<std::uint8_t> buildHeartbeatFrame(std::uint16_t sequence)
{
    std::vector<std::uint8_t> payload;
    appendU32Le(payload, 1000U);
    appendU16Le(payload, 0x0001U);

    std::vector<std::uint8_t> frame;
    frame.push_back(outdoor::mcu::kFrameSof0);
    frame.push_back(outdoor::mcu::kFrameSof1);
    frame.push_back(outdoor::mcu::kProtocolVersion);
    frame.push_back(static_cast<std::uint8_t>(outdoor::mcu::McuFrameType::Heartbeat));
    appendU16Le(frame, sequence);
    appendU16Le(frame, static_cast<std::uint16_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    appendU16Le(frame, outdoor::mcu::crc16Modbus(frame.data() + 2, frame.size() - 2));
    return frame;
}

} // namespace

int main()
{
    outdoor::mcu::McuFrameStreamDecoder decoder;
    std::vector<std::vector<std::uint8_t>> frames;
    std::string error;

    const auto frame1 = buildHeartbeatFrame(1U);
    const auto frame2 = buildHeartbeatFrame(2U);

    const std::vector<std::uint8_t> noise { 0x00U, 0x11U, 0x22U };
    assert(decoder.feed(noise.data(), noise.size(), frames, error));
    assert(frames.empty());

    assert(decoder.feed(frame1.data(), 3U, frames, error));
    assert(frames.empty());
    assert(decoder.feed(frame1.data() + 3U, frame1.size() - 3U, frames, error));
    assert(frames.size() == 1U);
    assert(frames[0] == frame1);

    std::vector<std::uint8_t> combined = frame2;
    combined.insert(combined.end(), frame1.begin(), frame1.end());
    assert(decoder.feed(combined.data(), combined.size(), frames, error));
    assert(frames.size() == 2U);
    assert(frames[0] == frame2);
    assert(frames[1] == frame1);

    outdoor::mcu::McuFrameParser parser;
    outdoor::mcu::McuFrame parsed;
    assert(parser.parseFrame(frames[0], parsed, error));
    assert(parsed.sequence == 2U);
    return 0;
}
