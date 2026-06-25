#include "sensor_hub_app.h"

#include "bsp/board_tick.h"
#include "bsp/board_uart.h"
#include "protocol/mcu_command_decoder_c.h"
#include "protocol/mcu_frame_builder_c.h"
#include "sensors/icm42688_provider_c.h"
#include "sensors/bmp390_provider_c.h"
#include "sensors/mmc5603_provider_c.h"
#include "sensors/mock_imu_provider_c.h"

#include <stddef.h>
#include <stdint.h>

enum {
    SENSOR_HUB_HEARTBEAT_PERIOD_MS = 1000U,
    SENSOR_HUB_IMU_PERIOD_MS = 10U,
    SENSOR_HUB_MAGNETOMETER_PERIOD_MS = 50U,
    SENSOR_HUB_BAROMETER_PERIOD_MS = 100U,
    SENSOR_HUB_MAX_RX_BYTES_PER_POLL = 32U,
    SENSOR_HUB_TX_BUFFER_SIZE = MCU_FRAME_MAX_SIZE,
    SENSOR_HUB_STATUS_ICM42688_READY = 0x0001U,
    SENSOR_HUB_STATUS_IMU_FALLBACK = 0x0002U,
    SENSOR_HUB_STATUS_IMU_ERROR = 0x0004U,
    SENSOR_HUB_STATUS_MMC5603_READY = 0x0008U,
    SENSOR_HUB_STATUS_MAGNETOMETER_ERROR = 0x0010U,
    SENSOR_HUB_STATUS_BMP390_READY = 0x0020U,
    SENSOR_HUB_STATUS_BAROMETER_ERROR = 0x0040U,
};

static uint32_t g_last_heartbeat_ms;
static uint32_t g_last_imu_ms;
static uint32_t g_last_magnetometer_ms;
static uint32_t g_last_barometer_ms;
static uint16_t g_sequence;
static mock_imu_provider_t g_mock_imu;
static icm42688_provider_t g_icm42688;
static mmc5603_provider_t g_mmc5603;
static bmp390_provider_t g_bmp390;
static mcu_command_decoder_c_t g_command_decoder;
static uint16_t g_status_flags;

static int time_elapsed(uint32_t now_ms, uint32_t last_ms, uint32_t period_ms)
{
    return (uint32_t)(now_ms - last_ms) >= period_ms;
}

static void send_frame_if_ready(const uint8_t* frame, size_t frame_len)
{
    if (frame != NULL && frame_len > 0U) {
        (void)board_uart_send_bytes(frame, frame_len);
    }
}

static void process_uart_commands(void)
{
    uint8_t byte = 0U;
    uint8_t frame[SENSOR_HUB_TX_BUFFER_SIZE];
    size_t frame_len = 0U;
    uint32_t count;

    for (count = 0U; count < SENSOR_HUB_MAX_RX_BYTES_PER_POLL; ++count) {
        mcu_command_c_t command;
        const int received = board_uart_receive_byte(&byte);
        if (received <= 0) {
            return;
        }

        if (mcu_command_decoder_feed_byte(&g_command_decoder, byte, &command) == 1 &&
            command.type == MCU_MSG_TYPE_COMMAND_PING) {
            ++g_sequence;
            if (mcu_build_command_ack_frame(g_sequence,
                                            command.type,
                                            0U,
                                            command.nonce,
                                            frame,
                                            sizeof(frame),
                                            &frame_len) == 0) {
                send_frame_if_ready(frame, frame_len);
            }
        }
    }
}

void sensor_hub_app_init(void)
{
    const uint32_t now_ms = board_get_tick_ms();
    g_last_heartbeat_ms = now_ms;
    g_last_imu_ms = now_ms;
    g_last_magnetometer_ms = now_ms;
    g_last_barometer_ms = now_ms;
    g_sequence = 0U;
    g_status_flags = 0U;
    mcu_command_decoder_init(&g_command_decoder);
    mock_imu_provider_init(&g_mock_imu, SENSOR_HUB_IMU_PERIOD_MS);
    if (icm42688_provider_init(&g_icm42688) == 0) {
        g_status_flags |= SENSOR_HUB_STATUS_ICM42688_READY;
    } else {
        g_status_flags |= SENSOR_HUB_STATUS_IMU_FALLBACK | SENSOR_HUB_STATUS_IMU_ERROR;
    }
    if (mmc5603_provider_init(&g_mmc5603) == 0) {
        g_status_flags |= SENSOR_HUB_STATUS_MMC5603_READY;
    } else {
        g_status_flags |= SENSOR_HUB_STATUS_MAGNETOMETER_ERROR;
    }
    if (bmp390_provider_init(&g_bmp390) == 0) {
        g_status_flags |= SENSOR_HUB_STATUS_BMP390_READY;
    } else {
        g_status_flags |= SENSOR_HUB_STATUS_BAROMETER_ERROR;
    }
}

void sensor_hub_app_poll(void)
{
    uint8_t frame[SENSOR_HUB_TX_BUFFER_SIZE];
    size_t frame_len = 0U;
    const uint32_t now_ms = board_get_tick_ms();

    process_uart_commands();

    if (time_elapsed(now_ms, g_last_heartbeat_ms, SENSOR_HUB_HEARTBEAT_PERIOD_MS)) {
        g_last_heartbeat_ms = now_ms;
        ++g_sequence;
        if (mcu_build_heartbeat_frame(g_sequence, now_ms, g_status_flags, frame, sizeof(frame), &frame_len) == 0) {
            send_frame_if_ready(frame, frame_len);
        }
    }

    if (time_elapsed(now_ms, g_last_imu_ms, SENSOR_HUB_IMU_PERIOD_MS)) {
        imu_sample_c_t sample;
        g_last_imu_ms += SENSOR_HUB_IMU_PERIOD_MS;
        if (icm42688_provider_read(&g_icm42688, now_ms, &sample) == 0) {
            g_status_flags &= (uint16_t)~(SENSOR_HUB_STATUS_IMU_FALLBACK | SENSOR_HUB_STATUS_IMU_ERROR);
            g_status_flags |= SENSOR_HUB_STATUS_ICM42688_READY;
        } else {
            mock_imu_provider_next(&g_mock_imu, now_ms, &sample);
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_ICM42688_READY;
            g_status_flags |= SENSOR_HUB_STATUS_IMU_FALLBACK | SENSOR_HUB_STATUS_IMU_ERROR;
        }
        ++g_sequence;
        if (mcu_build_imu_frame(g_sequence, &sample, frame, sizeof(frame), &frame_len) == 0) {
            send_frame_if_ready(frame, frame_len);
        }
    }

    if (time_elapsed(now_ms, g_last_magnetometer_ms, SENSOR_HUB_MAGNETOMETER_PERIOD_MS)) {
        magnetometer_sample_c_t sample;
        g_last_magnetometer_ms += SENSOR_HUB_MAGNETOMETER_PERIOD_MS;
        if (mmc5603_provider_read(&g_mmc5603, now_ms, &sample) == 0) {
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_MAGNETOMETER_ERROR;
            g_status_flags |= SENSOR_HUB_STATUS_MMC5603_READY;
            ++g_sequence;
            if (mcu_build_magnetometer_frame(g_sequence,
                                             &sample,
                                             frame,
                                             sizeof(frame),
                                             &frame_len) == 0) {
                send_frame_if_ready(frame, frame_len);
            }
        } else {
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_MMC5603_READY;
            g_status_flags |= SENSOR_HUB_STATUS_MAGNETOMETER_ERROR;
        }
    }

    if (time_elapsed(now_ms, g_last_barometer_ms, SENSOR_HUB_BAROMETER_PERIOD_MS)) {
        barometer_sample_c_t sample;
        const int result = bmp390_provider_read(&g_bmp390, now_ms, &sample);
        g_last_barometer_ms += SENSOR_HUB_BAROMETER_PERIOD_MS;
        if (result == 0) {
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_BAROMETER_ERROR;
            g_status_flags |= SENSOR_HUB_STATUS_BMP390_READY;
            ++g_sequence;
            if (mcu_build_barometer_frame(g_sequence,
                                          &sample,
                                          frame,
                                          sizeof(frame),
                                          &frame_len) == 0) {
                send_frame_if_ready(frame, frame_len);
            }
        } else if (result < 0) {
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_BMP390_READY;
            g_status_flags |= SENSOR_HUB_STATUS_BAROMETER_ERROR;
        }
    }
}
