#include "protocol/mcu_frame_builder.h"
#include "protocol/mcu_protocol.h"
#include "sensors/mock_imu_provider.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

std::uint16_t readU16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

} // namespace

int main()
{
    outdoor::f407::sensors::MockImuProvider provider(20);
    const auto sample = provider.nextSample();
    assert(sample.uptimeMs == 20);
    assert(sample.accelXMg == 13);
    assert(sample.accelYMg == -7);
    assert(sample.accelZMg == 1001);
    assert(sample.gyroXMdps == 1510);
    assert(sample.gyroYMdps == -2194);
    assert(sample.gyroZMdps == 324);
    assert(sample.temperatureCentiC == 2531);

    outdoor::f407::protocol::McuFrameBuilder builder;
    const auto frame = builder.buildImuFrame(7, sample);
    assert(frame.size() == outdoor::protocol::kFrameHeaderSize +
                               outdoor::protocol::kImuPayloadSize +
                               outdoor::protocol::kFrameCrcSize);
    assert(frame[0] == outdoor::protocol::kFrameSof0);
    assert(frame[1] == outdoor::protocol::kFrameSof1);
    assert(frame[2] == outdoor::protocol::kProtocolVersion);
    assert(frame[3] == outdoor::protocol::MSG_TYPE_SENSOR_IMU);
    assert(readU16Le(frame, 4) == 7);
    assert(readU16Le(frame, 6) == outdoor::protocol::kImuPayloadSize);

    const std::uint16_t expectedCrc = readU16Le(frame, frame.size() - 2);
    const std::uint16_t actualCrc = outdoor::protocol::crc16Modbus(frame.data() + 2, frame.size() - 4);
    assert(expectedCrc == actualCrc);
    return 0;
}
