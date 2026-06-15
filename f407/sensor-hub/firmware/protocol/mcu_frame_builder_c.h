#pragma once

#include "sensors/mock_imu_provider_c.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MCU_FRAME_SOF0 = 0xA5U,
    MCU_FRAME_SOF1 = 0x5AU,
    MCU_PROTOCOL_VERSION = 0x01U,
    MCU_MSG_TYPE_HEARTBEAT = 0x01U,
    MCU_MSG_TYPE_SENSOR_IMU = 0x11U,
    MCU_FRAME_HEADER_SIZE = 8U,
    MCU_FRAME_CRC_SIZE = 2U,
    MCU_HEARTBEAT_PAYLOAD_SIZE = 6U,
    MCU_IMU_PAYLOAD_SIZE = 24U,
    MCU_FRAME_MAX_PAYLOAD_SIZE = 64U,
    MCU_FRAME_MAX_SIZE = MCU_FRAME_HEADER_SIZE + MCU_FRAME_MAX_PAYLOAD_SIZE + MCU_FRAME_CRC_SIZE,
};

int mcu_build_heartbeat_frame(uint16_t sequence,
                              uint32_t uptime_ms,
                              uint16_t status_flags,
                              uint8_t* out_frame,
                              size_t out_capacity,
                              size_t* out_length);

int mcu_build_imu_frame(uint16_t sequence,
                        const imu_sample_c_t* sample,
                        uint8_t* out_frame,
                        size_t out_capacity,
                        size_t* out_length);

#ifdef __cplusplus
}
#endif
