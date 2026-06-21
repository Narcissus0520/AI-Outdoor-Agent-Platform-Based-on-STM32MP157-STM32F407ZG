#include "mcu/imu_payload_parser.h"
#include "mcu/mcu_command.h"
#include "mcu/mcu_frame_parser.h"
#include "mcu/mcu_protocol.h"
#include "protocol/imu_payload.h"

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

void appendI16Le(std::vector<std::uint8_t>& bytes, std::int16_t value)
{
    appendU16Le(bytes, static_cast<std::uint16_t>(value));
}

void appendI32Le(std::vector<std::uint8_t>& bytes, std::int32_t value)
{
    appendU32Le(bytes, static_cast<std::uint32_t>(value));
}

std::vector<std::uint8_t> buildImuPayload()
{
    std::vector<std::uint8_t> payload;
    payload.reserve(outdoor::mcu::kImuPayloadSize);
    appendU32Le(payload, 4020);
    appendI16Le(payload, 13);
    appendI16Le(payload, -7);
    appendI16Le(payload, 1001);
    appendI32Le(payload, 1510);
    appendI32Le(payload, -2194);
    appendI32Le(payload, 324);
    appendI16Le(payload, 2531);
    return payload;
}

std::vector<std::uint8_t> buildCommandAckPayload(std::uint8_t requestType,
                                                 std::uint8_t status,
                                                 std::uint32_t nonce)
{
    std::vector<std::uint8_t> payload;
    payload.reserve(outdoor::mcu::kCommandAckPayloadSize);
    payload.push_back(requestType);
    payload.push_back(status);
    appendU16Le(payload, 0U);
    appendU32Le(payload, nonce);
    return payload;
}

std::vector<std::uint8_t> buildFrame(outdoor::mcu::McuFrameType type,
                                     std::uint16_t sequence,
                                     const std::vector<std::uint8_t>& payload)
{
    std::vector<std::uint8_t> frame;
    frame.push_back(outdoor::mcu::kFrameSof0);
    frame.push_back(outdoor::mcu::kFrameSof1);
    frame.push_back(outdoor::mcu::kProtocolVersion);
    frame.push_back(static_cast<std::uint8_t>(type));
    appendU16Le(frame, sequence);
    appendU16Le(frame, static_cast<std::uint16_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());

    const std::uint16_t crc = outdoor::mcu::crc16Modbus(frame.data() + 2, frame.size() - 2);
    appendU16Le(frame, crc);
    return frame;
}

} // namespace

int main()
{
    const std::vector<std::uint8_t> crcVector {
        '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };
    assert(outdoor::mcu::crc16Modbus(crcVector.data(), crcVector.size()) == 0x4B37);

    outdoor::mcu::ImuPayloadParser imuParser;
    outdoor::protocol::ImuSample sample;
    std::string error;
    assert(imuParser.parse(buildImuPayload(), sample, error));
    assert(sample.uptimeMs == 4020);
    assert(sample.accelXMg == 13);
    assert(sample.accelYMg == -7);
    assert(sample.accelZMg == 1001);
    assert(sample.gyroXMdps == 1510);
    assert(sample.gyroYMdps == -2194);
    assert(sample.gyroZMdps == 324);
    assert(sample.temperatureCentiC == 2531);

    outdoor::mcu::McuFrameParser frameParser;
    outdoor::mcu::McuFrame frame;
    const auto imuFrame = buildFrame(outdoor::mcu::McuFrameType::SensorImu, 9, buildImuPayload());
    assert(frameParser.parseFrame(imuFrame, frame, error));
    assert(frame.type == outdoor::mcu::McuFrameType::SensorImu);
    assert(frame.sequence == 9);
    assert(frame.payload.size() == outdoor::mcu::kImuPayloadSize);

    outdoor::mcu::McuStatus status;
    assert(frameParser.applyFrame(frame, status, error));
    assert(status.imuSeen);
    assert(status.lastSequence == 9);
    assert(status.lastFrameType == "sensor_imu");
    assert(status.imuSample.gyroYMdps == -2194);

    const auto pingFrame = outdoor::mcu::buildPingCommandFrame(11U, outdoor::mcu::kDefaultPingNonce);
    assert(frameParser.parseFrame(pingFrame, frame, error));
    assert(frame.type == outdoor::mcu::McuFrameType::CommandPing);
    assert(frame.sequence == 11U);
    assert(frame.payload.size() == outdoor::mcu::kCommandPingPayloadSize);

    const auto ackFrame = buildFrame(
        outdoor::mcu::McuFrameType::CommandAck,
        12U,
        buildCommandAckPayload(
            static_cast<std::uint8_t>(outdoor::mcu::McuFrameType::CommandPing),
            0U,
            outdoor::mcu::kDefaultPingNonce));
    assert(frameParser.parseFrame(ackFrame, frame, error));
    assert(frameParser.applyFrame(frame, status, error));
    assert(status.commandAckSeen);
    assert(status.lastSequence == 12U);
    assert(status.lastFrameType == "command_ack");
    assert(status.commandAckRequestType == static_cast<std::uint8_t>(outdoor::mcu::McuFrameType::CommandPing));
    assert(status.commandAckStatus == 0U);
    assert(status.commandAckNonce == outdoor::mcu::kDefaultPingNonce);

    auto invalidCrcFrame = imuFrame;
    invalidCrcFrame.back() ^= 0xFFU;
    assert(!frameParser.parseFrame(invalidCrcFrame, frame, error));
    assert(error.find("CRC mismatch") != std::string::npos);

    const std::vector<std::uint8_t> shortPayload { 0x01, 0x02 };
    assert(!imuParser.parse(shortPayload, sample, error));
    assert(error == "IMU payload size is invalid");
    return 0;
}
