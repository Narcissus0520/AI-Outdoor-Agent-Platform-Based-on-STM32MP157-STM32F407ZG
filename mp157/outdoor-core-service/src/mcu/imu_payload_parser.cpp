#include "mcu/imu_payload_parser.h"

#include "mcu/mcu_protocol.h"

namespace outdoor::mcu {

namespace {

std::uint16_t readU16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::uint32_t readU32Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::int16_t readI16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::int16_t>(readU16Le(bytes, offset));
}

std::int32_t readI32Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U));
}

} // namespace

bool ImuPayloadParser::parse(const std::vector<std::uint8_t>& payload,
                             outdoor::protocol::ImuSample& sample,
                             std::string& error) const
{
    if (payload.size() != kImuPayloadSize) {
        error = "IMU payload size is invalid";
        return false;
    }

    sample.uptimeMs = readU32Le(payload, 0);
    sample.accelXMg = readI16Le(payload, 4);
    sample.accelYMg = readI16Le(payload, 6);
    sample.accelZMg = readI16Le(payload, 8);
    sample.gyroXMdps = readI32Le(payload, 10);
    sample.gyroYMdps = readI32Le(payload, 14);
    sample.gyroZMdps = readI32Le(payload, 18);
    sample.temperatureCentiC = readI16Le(payload, 22);
    error.clear();
    return true;
}

} // namespace outdoor::mcu
