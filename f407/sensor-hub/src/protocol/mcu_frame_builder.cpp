#include "protocol/mcu_frame_builder.h"

#include "protocol/mcu_protocol.h"

namespace outdoor::f407::protocol {

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

} // namespace

std::vector<std::uint8_t> McuFrameBuilder::buildImuFrame(
    std::uint16_t sequence,
    const outdoor::protocol::ImuSample& sample) const
{
    std::vector<std::uint8_t> bytes;
    bytes.reserve(outdoor::protocol::kFrameHeaderSize +
                  outdoor::protocol::kImuPayloadSize +
                  outdoor::protocol::kFrameCrcSize);

    bytes.push_back(outdoor::protocol::kFrameSof0);
    bytes.push_back(outdoor::protocol::kFrameSof1);
    bytes.push_back(outdoor::protocol::kProtocolVersion);
    bytes.push_back(outdoor::protocol::MSG_TYPE_SENSOR_IMU);
    appendU16Le(bytes, sequence);
    appendU16Le(bytes, static_cast<std::uint16_t>(outdoor::protocol::kImuPayloadSize));

    appendU32Le(bytes, sample.uptimeMs);
    appendI16Le(bytes, sample.accelXMg);
    appendI16Le(bytes, sample.accelYMg);
    appendI16Le(bytes, sample.accelZMg);
    appendI32Le(bytes, sample.gyroXMdps);
    appendI32Le(bytes, sample.gyroYMdps);
    appendI32Le(bytes, sample.gyroZMdps);
    appendI16Le(bytes, sample.temperatureCentiC);

    const std::uint16_t crc = outdoor::protocol::crc16Modbus(
        bytes.data() + 2,
        bytes.size() - 2);
    appendU16Le(bytes, crc);
    return bytes;
}

} // namespace outdoor::f407::protocol
