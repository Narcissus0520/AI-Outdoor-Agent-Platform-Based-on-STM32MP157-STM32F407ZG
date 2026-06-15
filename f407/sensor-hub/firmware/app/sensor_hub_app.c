#include "sensor_hub_app.h"

#include "bsp/board_tick.h"
#include "bsp/board_uart.h"
#include "protocol/mcu_frame_builder_c.h"
#include "sensors/mock_imu_provider_c.h"

#include <stddef.h>
#include <stdint.h>

enum {
    SENSOR_HUB_HEARTBEAT_PERIOD_MS = 1000U,
    SENSOR_HUB_IMU_PERIOD_MS = 100U,
    SENSOR_HUB_TX_BUFFER_SIZE = MCU_FRAME_MAX_SIZE,
};

static uint32_t g_last_heartbeat_ms;
static uint32_t g_last_imu_ms;
static uint16_t g_sequence;
static mock_imu_provider_t g_mock_imu;

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

void sensor_hub_app_init(void)
{
    const uint32_t now_ms = board_get_tick_ms();
    g_last_heartbeat_ms = now_ms;
    g_last_imu_ms = now_ms;
    g_sequence = 0U;
    mock_imu_provider_init(&g_mock_imu, SENSOR_HUB_IMU_PERIOD_MS);
}

void sensor_hub_app_poll(void)
{
    uint8_t frame[SENSOR_HUB_TX_BUFFER_SIZE];
    size_t frame_len = 0U;
    const uint32_t now_ms = board_get_tick_ms();

    if (time_elapsed(now_ms, g_last_heartbeat_ms, SENSOR_HUB_HEARTBEAT_PERIOD_MS)) {
        g_last_heartbeat_ms = now_ms;
        ++g_sequence;
        if (mcu_build_heartbeat_frame(g_sequence, now_ms, 0U, frame, sizeof(frame), &frame_len) == 0) {
            send_frame_if_ready(frame, frame_len);
        }
    }

    if (time_elapsed(now_ms, g_last_imu_ms, SENSOR_HUB_IMU_PERIOD_MS)) {
        imu_sample_c_t sample;
        g_last_imu_ms = now_ms;
        mock_imu_provider_next(&g_mock_imu, now_ms, &sample);
        ++g_sequence;
        if (mcu_build_imu_frame(g_sequence, &sample, frame, sizeof(frame), &frame_len) == 0) {
            send_frame_if_ready(frame, frame_len);
        }
    }
}
