#include "protocol/mcu_frame_builder_c.h"

#include "protocol/crc16_modbus_c.h"

#include <stddef.h>
#include <stdint.h>

static void write_u16_le(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static void write_u32_le(uint8_t* out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8U) & 0xFFU);
    out[2] = (uint8_t)((value >> 16U) & 0xFFU);
    out[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static void write_i16_le(uint8_t* out, int16_t value)
{
    write_u16_le(out, (uint16_t)value);
}

static void write_i32_le(uint8_t* out, int32_t value)
{
    write_u32_le(out, (uint32_t)value);
}

static int begin_frame(uint8_t msg_type,
                       uint16_t sequence,
                       uint16_t payload_size,
                       uint8_t* out_frame,
                       size_t out_capacity)
{
    const size_t frame_size = MCU_FRAME_HEADER_SIZE + payload_size + MCU_FRAME_CRC_SIZE;

    if (out_frame == 0 || out_capacity < frame_size || payload_size > MCU_FRAME_MAX_PAYLOAD_SIZE) {
        return -1;
    }

    out_frame[0] = MCU_FRAME_SOF0;
    out_frame[1] = MCU_FRAME_SOF1;
    out_frame[2] = MCU_PROTOCOL_VERSION;
    out_frame[3] = msg_type;
    write_u16_le(&out_frame[4], sequence);
    write_u16_le(&out_frame[6], payload_size);
    return 0;
}

static void finish_frame(uint8_t* out_frame, uint16_t payload_size, size_t* out_length)
{
    const size_t crc_offset = MCU_FRAME_HEADER_SIZE + payload_size;
    const uint16_t crc = crc16_modbus_c(&out_frame[2], MCU_FRAME_HEADER_SIZE - 2U + payload_size);
    write_u16_le(&out_frame[crc_offset], crc);
    if (out_length != 0) {
        *out_length = crc_offset + MCU_FRAME_CRC_SIZE;
    }
}

int mcu_build_heartbeat_frame(uint16_t sequence,
                              uint32_t uptime_ms,
                              uint16_t status_flags,
                              uint8_t* out_frame,
                              size_t out_capacity,
                              size_t* out_length)
{
    uint8_t* payload;

    if (begin_frame(MCU_MSG_TYPE_HEARTBEAT,
                    sequence,
                    MCU_HEARTBEAT_PAYLOAD_SIZE,
                    out_frame,
                    out_capacity) != 0) {
        return -1;
    }

    payload = &out_frame[MCU_FRAME_HEADER_SIZE];
    write_u32_le(&payload[0], uptime_ms);
    write_u16_le(&payload[4], status_flags);
    finish_frame(out_frame, MCU_HEARTBEAT_PAYLOAD_SIZE, out_length);
    return 0;
}

int mcu_build_sensor_hub_diagnostics_frame(
    uint16_t sequence,
    const mcu_sensor_hub_diagnostics_c_t* diagnostics,
    uint8_t* out_frame,
    size_t out_capacity,
    size_t* out_length)
{
    uint8_t* payload;

    if (diagnostics == 0 ||
        begin_frame(MCU_MSG_TYPE_SENSOR_HUB_DIAGNOSTICS,
                    sequence,
                    MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE,
                    out_frame,
                    out_capacity) != 0) {
        return -1;
    }

    payload = &out_frame[MCU_FRAME_HEADER_SIZE];
    write_u32_le(&payload[0], diagnostics->uptime_ms);
    write_u32_le(&payload[4], diagnostics->i2c_recovery_count);
    write_u32_le(&payload[8], diagnostics->i2c_transaction_failure_count);
    write_u32_le(&payload[12], diagnostics->i2c_last_hal_error);
    write_u32_le(&payload[16], diagnostics->fifo_overflow_count);
    write_u32_le(&payload[20], diagnostics->fifo_malformed_packet_count);
    write_u32_le(&payload[24], diagnostics->fifo_empty_event_count);
    write_u32_le(&payload[28], diagnostics->fifo_drain_stall_count);
    write_u32_le(&payload[32], diagnostics->fifo_skipped_packet_count);
    write_u16_le(&payload[36], diagnostics->i2c_last_length);
    payload[38] = diagnostics->i2c_last_device_address;
    payload[39] = diagnostics->i2c_last_register_address;
    payload[40] = diagnostics->i2c_last_operation;
    payload[41] = diagnostics->i2c_last_hal_status;
    payload[42] = diagnostics->icm42688_init_error_step;
    payload[43] = MCU_SENSOR_HUB_DIAGNOSTICS_EXTENSION_VERSION;
    write_u32_le(&payload[44], diagnostics->uart4_rx_drop_count);
    finish_frame(out_frame, MCU_SENSOR_HUB_DIAGNOSTICS_PAYLOAD_SIZE, out_length);
    return 0;
}

int mcu_build_imu_frame(uint16_t sequence,
                        const imu_sample_c_t* sample,
                        uint8_t* out_frame,
                        size_t out_capacity,
                        size_t* out_length)
{
    uint8_t* payload;

    if (sample == 0) {
        return -1;
    }

    if (begin_frame(MCU_MSG_TYPE_SENSOR_IMU,
                    sequence,
                    MCU_IMU_PAYLOAD_SIZE,
                    out_frame,
                    out_capacity) != 0) {
        return -1;
    }

    payload = &out_frame[MCU_FRAME_HEADER_SIZE];
    write_u32_le(&payload[0], sample->uptime_ms);
    write_i16_le(&payload[4], sample->accel_x_mg);
    write_i16_le(&payload[6], sample->accel_y_mg);
    write_i16_le(&payload[8], sample->accel_z_mg);
    write_i32_le(&payload[10], sample->gyro_x_mdps);
    write_i32_le(&payload[14], sample->gyro_y_mdps);
    write_i32_le(&payload[18], sample->gyro_z_mdps);
    write_i16_le(&payload[22], sample->temperature_centi_c);
    finish_frame(out_frame, MCU_IMU_PAYLOAD_SIZE, out_length);
    return 0;
}

int mcu_build_magnetometer_frame(uint16_t sequence,
                                 const magnetometer_sample_c_t* sample,
                                 uint8_t* out_frame,
                                 size_t out_capacity,
                                 size_t* out_length)
{
    uint8_t* payload;

    if (sample == 0) {
        return -1;
    }

    if (begin_frame(MCU_MSG_TYPE_SENSOR_MAGNETOMETER,
                    sequence,
                    MCU_MAGNETOMETER_PAYLOAD_SIZE,
                    out_frame,
                    out_capacity) != 0) {
        return -1;
    }

    payload = &out_frame[MCU_FRAME_HEADER_SIZE];
    write_u32_le(&payload[0], sample->uptime_ms);
    write_i32_le(&payload[4], sample->magnetic_x_nt);
    write_i32_le(&payload[8], sample->magnetic_y_nt);
    write_i32_le(&payload[12], sample->magnetic_z_nt);
    finish_frame(out_frame, MCU_MAGNETOMETER_PAYLOAD_SIZE, out_length);
    return 0;
}

int mcu_build_barometer_frame(uint16_t sequence,
                              const barometer_sample_c_t* sample,
                              uint8_t* out_frame,
                              size_t out_capacity,
                              size_t* out_length)
{
    uint8_t* payload;

    if (sample == 0) {
        return -1;
    }

    if (begin_frame(MCU_MSG_TYPE_SENSOR_BAROMETER,
                    sequence,
                    MCU_BAROMETER_PAYLOAD_SIZE,
                    out_frame,
                    out_capacity) != 0) {
        return -1;
    }

    payload = &out_frame[MCU_FRAME_HEADER_SIZE];
    write_u32_le(&payload[0], sample->uptime_ms);
    write_u32_le(&payload[4], sample->pressure_pa);
    write_i16_le(&payload[8], sample->temperature_centi_c);
    finish_frame(out_frame, MCU_BAROMETER_PAYLOAD_SIZE, out_length);
    return 0;
}

int mcu_build_command_ack_frame(uint16_t sequence,
                                uint8_t request_type,
                                uint8_t status,
                                uint32_t nonce,
                                uint8_t* out_frame,
                                size_t out_capacity,
                                size_t* out_length)
{
    uint8_t* payload;

    if (begin_frame(MCU_MSG_TYPE_COMMAND_ACK,
                    sequence,
                    MCU_COMMAND_ACK_PAYLOAD_SIZE,
                    out_frame,
                    out_capacity) != 0) {
        return -1;
    }

    payload = &out_frame[MCU_FRAME_HEADER_SIZE];
    payload[0] = request_type;
    payload[1] = status;
    write_u16_le(&payload[2], 0U);
    write_u32_le(&payload[4], nonce);
    finish_frame(out_frame, MCU_COMMAND_ACK_PAYLOAD_SIZE, out_length);
    return 0;
}
