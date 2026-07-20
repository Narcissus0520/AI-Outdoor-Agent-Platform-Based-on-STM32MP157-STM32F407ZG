#pragma once

#include "sensors/imu_sample_c.h"
#include "sensors/barometer_sample_c.h"
#include "sensors/magnetometer_sample_c.h"

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
    MCU_MSG_TYPE_SENSOR_HUB_DIAGNOSTICS = 0x02U,
    MCU_MSG_TYPE_SENSOR_IMU = 0x11U,
    MCU_MSG_TYPE_SENSOR_MAGNETOMETER = 0x12U,
    MCU_MSG_TYPE_SENSOR_BAROMETER = 0x13U,
    MCU_MSG_TYPE_COMMAND_PING = 0x80U,
    MCU_MSG_TYPE_COMMAND_ACK = 0x81U,
    MCU_FRAME_HEADER_SIZE = 8U,
    MCU_FRAME_CRC_SIZE = 2U,
    MCU_HEARTBEAT_PAYLOAD_SIZE = 6U,
    MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE = 48U,
    MCU_SENSOR_HUB_DIAGNOSTICS_EXTENSION_VERSION = 1U,
    MCU_IMU_PAYLOAD_SIZE = 24U,
    MCU_MAGNETOMETER_PAYLOAD_SIZE = 16U,
    MCU_BAROMETER_PAYLOAD_SIZE = 10U,
    MCU_COMMAND_PING_PAYLOAD_SIZE = 4U,
    MCU_COMMAND_ACK_PAYLOAD_SIZE = 8U,
    MCU_FRAME_MAX_PAYLOAD_SIZE = 64U,
    MCU_FRAME_MAX_SIZE = MCU_FRAME_HEADER_SIZE + MCU_FRAME_MAX_PAYLOAD_SIZE + MCU_FRAME_CRC_SIZE,
};

typedef struct {
    uint32_t uptime_ms;
    uint32_t i2c_recovery_count;
    uint32_t i2c_transaction_failure_count;
    uint32_t i2c_last_hal_error;
    uint32_t fifo_overflow_count;
    uint32_t fifo_malformed_packet_count;
    uint32_t fifo_empty_event_count;
    uint32_t fifo_drain_stall_count;
    uint32_t fifo_skipped_packet_count;
    uint16_t i2c_last_length;
    uint8_t i2c_last_device_address;
    uint8_t i2c_last_register_address;
    uint8_t i2c_last_operation;
    uint8_t i2c_last_hal_status;
    uint8_t icm42688_init_error_step;
    uint32_t uart4_rx_drop_count;
} mcu_sensor_hub_diagnostics_c_t;

int mcu_build_heartbeat_frame(uint16_t sequence,
                              uint32_t uptime_ms,
                              uint16_t status_flags,
                              uint8_t* out_frame,
                              size_t out_capacity,
                              size_t* out_length);

int mcu_build_sensor_hub_diagnostics_frame(
    uint16_t sequence,
    const mcu_sensor_hub_diagnostics_c_t* diagnostics,
    uint8_t* out_frame,
    size_t out_capacity,
    size_t* out_length);

int mcu_build_imu_frame(uint16_t sequence,
                        const imu_sample_c_t* sample,
                        uint8_t* out_frame,
                        size_t out_capacity,
                        size_t* out_length);

int mcu_build_magnetometer_frame(uint16_t sequence,
                                 const magnetometer_sample_c_t* sample,
                                 uint8_t* out_frame,
                                 size_t out_capacity,
                                 size_t* out_length);

int mcu_build_barometer_frame(uint16_t sequence,
                              const barometer_sample_c_t* sample,
                              uint8_t* out_frame,
                              size_t out_capacity,
                              size_t* out_length);

int mcu_build_command_ack_frame(uint16_t sequence,
                                uint8_t request_type,
                                uint8_t status,
                                uint32_t nonce,
                                uint8_t* out_frame,
                                size_t out_capacity,
                                size_t* out_length);

#ifdef __cplusplus
}
#endif
