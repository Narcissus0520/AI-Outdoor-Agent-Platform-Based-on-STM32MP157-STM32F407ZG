#include "protocol/crc16_modbus_c.h"
#include "protocol/mcu_frame_builder_c.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace {

std::uint16_t readU16Le(const std::uint8_t* bytes)
{
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8U);
}

std::uint32_t readU32Le(const std::uint8_t* bytes)
{
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

} // namespace

int main()
{
    mcu_sensor_hub_diagnostics_c_t diagnostics {};
    diagnostics.uptime_ms = 5000U;
    diagnostics.i2c_recovery_count = 12U;
    diagnostics.icm42688_init_error_step = 3U;
    diagnostics.uart4_rx_drop_count = 0x01020304U;

    std::uint8_t frame[MCU_FRAME_MAX_SIZE] {};
    std::size_t frameLength = 0U;
    assert(mcu_build_sensor_hub_diagnostics_frame(
               0x1234U,
               &diagnostics,
               frame,
               sizeof(frame),
               &frameLength) == 0);
    assert(frameLength == MCU_FRAME_HEADER_SIZE + MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE
                              + MCU_FRAME_CRC_SIZE);
    assert(frame[0] == MCU_FRAME_SOF0);
    assert(frame[1] == MCU_FRAME_SOF1);
    assert(frame[2] == MCU_PROTOCOL_VERSION);
    assert(frame[3] == MCU_MSG_TYPE_SENSOR_HUB_DIAGNOSTICS);
    assert(readU16Le(&frame[4]) == 0x1234U);
    assert(readU16Le(&frame[6]) == MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE);

    const std::uint8_t* payload = &frame[MCU_FRAME_HEADER_SIZE];
    assert(readU32Le(&payload[0]) == 5000U);
    assert(readU32Le(&payload[4]) == 12U);
    assert(payload[42] == 3U);
    assert(payload[43] == MCU_SENSOR_HUB_DIAGNOSTICS_EXTENSION_VERSION);
    assert(readU32Le(&payload[44]) == 0x01020304U);

    const std::uint16_t expectedCrc = crc16_modbus_c(
        &frame[2],
        MCU_FRAME_HEADER_SIZE - 2U + MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE);
    assert(readU16Le(&frame[MCU_FRAME_HEADER_SIZE + MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE])
           == expectedCrc);

    std::cout << "mcu_frame_builder_tests passed\n";
    return 0;
}
