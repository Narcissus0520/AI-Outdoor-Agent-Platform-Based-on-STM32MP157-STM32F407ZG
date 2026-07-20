#include "sensor_hub_app.h"

#include "bsp/board_icm42688_gpio.h"
#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"
#include "bsp/board_uart.h"
#include "protocol/mcu_command_decoder_c.h"
#include "protocol/mcu_frame_builder_c.h"
#include "sensors/icm42688_provider_c.h"
#include "sensors/imu_timestamp_normalizer_c.h"
#include "sensors/bmp390_provider_c.h"
#include "sensors/mmc5603_provider_c.h"
#include "sensors/mock_imu_provider_c.h"
#include "sensors/sensor_sample_filter_c.h"

#include <stddef.h>
#include <stdint.h>

#ifndef SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
#define SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC 0
#endif

enum {
    SENSOR_HUB_HEARTBEAT_PERIOD_MS = 1000U,
    SENSOR_HUB_IMU_FALLBACK_PERIOD_MS = 10U,
    SENSOR_HUB_IMU_FIFO_READ_MIN_PERIOD_MS = 30U,
    SENSOR_HUB_IMU_EVENT_TIMEOUT_MS = 250U,
    SENSOR_HUB_IMU_REINIT_PERIOD_MS = 1000U,
    SENSOR_HUB_IMU_REINIT_FAILURE_THRESHOLD = 3U,
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
    SENSOR_HUB_STATUS_ICM42688_INT1_ACTIVE = 0x0080U,
    SENSOR_HUB_STATUS_ICM42688_FIFO_ACTIVE = 0x0100U,
    SENSOR_HUB_STATUS_ICM42688_FIFO_ERROR = 0x0200U,
    SENSOR_HUB_STATUS_UART4_TX_OVERFLOW = 0x0400U,
    SENSOR_HUB_STATUS_USART1_TX_OVERFLOW = 0x0800U,
    SENSOR_HUB_STATUS_ICM42688_INIT_ERROR = 0x1000U,
    SENSOR_HUB_STATUS_UART4_RX_OVERFLOW = 0x2000U,
    SENSOR_HUB_STATUS_I2C_TRANSACTION_FAILURE = 0x4000U,
    SENSOR_HUB_STATUS_DIAGNOSTIC_IMAGE = 0x8000U,
};

static uint32_t g_last_heartbeat_ms;
static uint32_t g_last_imu_event_ms;
static uint32_t g_last_imu_fifo_read_attempt_ms;
static uint32_t g_last_imu_fallback_ms;
static uint32_t g_last_imu_reinit_ms;
#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
static uint32_t g_last_magnetometer_ms;
static uint32_t g_last_barometer_ms;
#endif
static uint16_t g_sequence;
static mock_imu_provider_t g_mock_imu;
static icm42688_provider_t g_icm42688;
#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
static mmc5603_provider_t g_mmc5603;
static bmp390_provider_t g_bmp390;
#endif
static imu_sample_filter_c_t g_imu_filter;
static imu_timestamp_normalizer_c_t g_imu_timestamp_normalizer;
#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
static magnetometer_sample_filter_c_t g_magnetometer_filter;
static barometer_sample_filter_c_t g_barometer_filter;
#endif
static mcu_command_decoder_c_t g_command_decoder;
static uint16_t g_status_flags;
static uint32_t g_imu_read_failure_count;
static int g_imu_filter_has_real_source;

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

static void set_imu_ready_status(void)
{
    g_status_flags &= (uint16_t)~(SENSOR_HUB_STATUS_IMU_FALLBACK | SENSOR_HUB_STATUS_IMU_ERROR);
    g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_ICM42688_INIT_ERROR;
    g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_ICM42688_FIFO_ERROR;
    g_status_flags |= SENSOR_HUB_STATUS_ICM42688_READY |
                      SENSOR_HUB_STATUS_ICM42688_INT1_ACTIVE |
                      SENSOR_HUB_STATUS_ICM42688_FIFO_ACTIVE;
}

static void set_imu_fallback_status(int interrupt_active)
{
    g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_ICM42688_READY;
    if (interrupt_active == 0) {
        g_status_flags &= (uint16_t)~(SENSOR_HUB_STATUS_ICM42688_INT1_ACTIVE |
                                      SENSOR_HUB_STATUS_ICM42688_FIFO_ACTIVE);
    }
    g_status_flags |= SENSOR_HUB_STATUS_IMU_FALLBACK | SENSOR_HUB_STATUS_IMU_ERROR;
}

static void send_imu_sample(imu_sample_c_t* sample, uint8_t* frame, size_t frame_capacity)
{
    size_t frame_len = 0U;

    ++g_sequence;
    if (mcu_build_imu_frame(g_sequence, sample, frame, frame_capacity, &frame_len) == 0) {
        send_frame_if_ready(frame, frame_len);
    }
}

static void reset_real_imu_filter(void)
{
    imu_sample_filter_reset(&g_imu_filter);
    g_imu_filter_has_real_source = 0;
}

static void try_reinitialize_icm42688(uint32_t now_ms)
{
    if (g_imu_read_failure_count < SENSOR_HUB_IMU_REINIT_FAILURE_THRESHOLD ||
        !time_elapsed(now_ms, g_last_imu_reinit_ms, SENSOR_HUB_IMU_REINIT_PERIOD_MS)) {
        return;
    }

    g_last_imu_reinit_ms = now_ms;
    set_imu_fallback_status(0);
    reset_real_imu_filter();
    if (icm42688_provider_init(&g_icm42688) == 0) {
        board_icm42688_discard_data_ready_events();
        g_last_imu_event_ms = board_get_tick_ms();
        g_last_imu_fifo_read_attempt_ms = g_last_imu_event_ms;
        g_imu_read_failure_count = 0U;
    } else {
        g_status_flags |= SENSOR_HUB_STATUS_ICM42688_INIT_ERROR;
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

static void update_uart_status(void)
{
    if (board_uart_primary_tx_drop_count() > 0U) {
        g_status_flags |= SENSOR_HUB_STATUS_UART4_TX_OVERFLOW;
    }
    if (board_uart_diagnostic_tx_drop_count() > 0U) {
        g_status_flags |= SENSOR_HUB_STATUS_USART1_TX_OVERFLOW;
    }
    if (board_uart_rx_drop_count() > 0U) {
        g_status_flags |= SENSOR_HUB_STATUS_UART4_RX_OVERFLOW;
    }
    if (board_i2c_transaction_failure_active() != 0) {
        g_status_flags |= SENSOR_HUB_STATUS_I2C_TRANSACTION_FAILURE;
    } else {
        g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_I2C_TRANSACTION_FAILURE;
    }
}

static void send_sensor_hub_diagnostics(uint32_t now_ms,
                                        uint8_t* frame,
                                        size_t frame_capacity)
{
    board_i2c_diagnostics_t i2c_diagnostics;
    mcu_sensor_hub_diagnostics_c_t diagnostics;
    size_t frame_len = 0U;

    board_i2c_get_diagnostics(&i2c_diagnostics);
    diagnostics.uptime_ms = now_ms;
    diagnostics.i2c_recovery_count = i2c_diagnostics.recovery_count;
    diagnostics.i2c_transaction_failure_count =
        i2c_diagnostics.transaction_failure_count;
    diagnostics.i2c_last_hal_error = i2c_diagnostics.last_hal_error;
    diagnostics.fifo_overflow_count = g_icm42688.fifo_overflow_count;
    diagnostics.fifo_malformed_packet_count =
        g_icm42688.fifo_malformed_packet_count;
    diagnostics.fifo_empty_event_count = g_icm42688.fifo_empty_event_count;
    diagnostics.fifo_drain_stall_count = g_icm42688.fifo_drain_stall_count;
    diagnostics.fifo_skipped_packet_count = g_icm42688.fifo_skipped_packet_count;
    diagnostics.i2c_last_length = i2c_diagnostics.last_length;
    diagnostics.i2c_last_device_address = i2c_diagnostics.last_device_address;
    diagnostics.i2c_last_register_address = i2c_diagnostics.last_register_address;
    diagnostics.i2c_last_operation = i2c_diagnostics.last_operation;
    diagnostics.i2c_last_hal_status = i2c_diagnostics.last_hal_status;
    diagnostics.icm42688_init_error_step = g_icm42688.init_error_step;
    diagnostics.uart4_rx_drop_count = board_uart_rx_drop_count();

    ++g_sequence;
    if (mcu_build_sensor_hub_diagnostics_frame(g_sequence,
                                                &diagnostics,
                                                frame,
                                                frame_capacity,
                                                &frame_len) == 0) {
        (void)board_uart_send_bytes(frame, frame_len);
        (void)board_uart_send_diagnostic_bytes(frame, frame_len);
    }
}

void sensor_hub_app_init(void)
{
    const uint32_t now_ms = board_get_tick_ms();
    g_last_heartbeat_ms = now_ms;
    g_last_imu_event_ms = now_ms;
    g_last_imu_fifo_read_attempt_ms = now_ms;
    g_last_imu_fallback_ms = now_ms;
    g_last_imu_reinit_ms = now_ms;
#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
    g_last_magnetometer_ms = now_ms;
    g_last_barometer_ms = now_ms;
#endif
    g_sequence = 0U;
#if SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
    g_status_flags = SENSOR_HUB_STATUS_DIAGNOSTIC_IMAGE;
#else
    g_status_flags = 0U;
#endif
    g_imu_read_failure_count = 0U;
    g_imu_filter_has_real_source = 0;
    (void)board_uart_init();
    (void)board_icm42688_interrupt_init();
    mcu_command_decoder_init(&g_command_decoder);
    mock_imu_provider_init(&g_mock_imu, SENSOR_HUB_IMU_FALLBACK_PERIOD_MS);
    imu_sample_filter_init(&g_imu_filter);
    imu_timestamp_normalizer_init(&g_imu_timestamp_normalizer);
#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
    magnetometer_sample_filter_init(&g_magnetometer_filter);
    barometer_sample_filter_init(&g_barometer_filter);
#endif
    icm42688_provider_reset(&g_icm42688);
#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
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
#endif
    /* Start the FIFO last so other sensor setup cannot create a startup backlog. */
    if (icm42688_provider_init(&g_icm42688) == 0) {
        g_status_flags |= SENSOR_HUB_STATUS_ICM42688_READY;
        board_icm42688_discard_data_ready_events();
        g_last_imu_event_ms = board_get_tick_ms();
        g_last_imu_fifo_read_attempt_ms = g_last_imu_event_ms;
        g_last_imu_fallback_ms = g_last_imu_event_ms;
    } else {
        g_status_flags |= SENSOR_HUB_STATUS_IMU_FALLBACK | SENSOR_HUB_STATUS_IMU_ERROR;
        g_status_flags |= SENSOR_HUB_STATUS_ICM42688_INIT_ERROR;
    }

    /* Address probing may intentionally NACK. Runtime diagnostics start here. */
    board_i2c_reset_diagnostics();
}

void sensor_hub_app_poll(void)
{
    uint8_t frame[SENSOR_HUB_TX_BUFFER_SIZE];
    size_t frame_len = 0U;
    const uint32_t now_ms = board_get_tick_ms();

    board_uart_poll();
    update_uart_status();
    process_uart_commands();

    if (time_elapsed(now_ms, g_last_heartbeat_ms, SENSOR_HUB_HEARTBEAT_PERIOD_MS)) {
        g_last_heartbeat_ms = now_ms;
        ++g_sequence;
        if (mcu_build_heartbeat_frame(g_sequence, now_ms, g_status_flags, frame, sizeof(frame), &frame_len) == 0) {
            send_frame_if_ready(frame, frame_len);
        }
        send_sensor_hub_diagnostics(now_ms, frame, sizeof(frame));
    }

    if (board_icm42688_take_data_ready_event() > 0 &&
        time_elapsed(now_ms,
                     g_last_imu_fifo_read_attempt_ms,
                     SENSOR_HUB_IMU_FIFO_READ_MIN_PERIOD_MS)) {
        imu_sample_c_t samples[ICM42688_FIFO_BATCH_CAPACITY];
        size_t sample_count = 0U;
        size_t sample_index;
        int read_result;

        g_last_imu_fifo_read_attempt_ms = now_ms;
        read_result = icm42688_provider_read_fifo(&g_icm42688,
                                                   now_ms,
                                                   samples,
                                                   ICM42688_FIFO_BATCH_CAPACITY,
                                                   &sample_count);
        if (read_result == 0 && sample_count > 0U) {
            g_last_imu_event_ms = now_ms;
            g_last_imu_fallback_ms = now_ms;
            if (g_imu_filter_has_real_source == 0) {
                imu_sample_filter_reset(&g_imu_filter);
                g_imu_filter_has_real_source = 1;
            }
            (void)imu_timestamp_normalizer_apply(&g_imu_timestamp_normalizer,
                                                 samples,
                                                 sample_count,
                                                 SENSOR_HUB_IMU_FALLBACK_PERIOD_MS);
            for (sample_index = 0U; sample_index < sample_count; ++sample_index) {
                imu_sample_filter_apply(&g_imu_filter, &samples[sample_index]);
                send_imu_sample(&samples[sample_index], frame, sizeof(frame));
            }
            g_imu_read_failure_count = 0U;
            set_imu_ready_status();
        } else if (read_result < 0) {
            ++g_imu_read_failure_count;
            if (read_result <= ICM42688_PROVIDER_FIFO_OVERFLOW) {
                g_status_flags |= SENSOR_HUB_STATUS_ICM42688_FIFO_ERROR;
            }
            if (g_imu_read_failure_count >= SENSOR_HUB_IMU_REINIT_FAILURE_THRESHOLD) {
                set_imu_fallback_status(1);
                reset_real_imu_filter();
            }
        }
    }

    if (time_elapsed(now_ms, g_last_imu_event_ms, SENSOR_HUB_IMU_EVENT_TIMEOUT_MS)) {
        set_imu_fallback_status(0);
        reset_real_imu_filter();
        if (g_imu_read_failure_count < SENSOR_HUB_IMU_REINIT_FAILURE_THRESHOLD) {
            g_imu_read_failure_count = SENSOR_HUB_IMU_REINIT_FAILURE_THRESHOLD;
        }
    }

    try_reinitialize_icm42688(now_ms);

    if ((g_status_flags & SENSOR_HUB_STATUS_ICM42688_READY) == 0U &&
        time_elapsed(now_ms, g_last_imu_fallback_ms, SENSOR_HUB_IMU_FALLBACK_PERIOD_MS)) {
        imu_sample_c_t sample;
        g_last_imu_fallback_ms = now_ms;
        mock_imu_provider_next(&g_mock_imu, now_ms, &sample);
        (void)imu_timestamp_normalizer_apply(&g_imu_timestamp_normalizer,
                                             &sample,
                                             1U,
                                             SENSOR_HUB_IMU_FALLBACK_PERIOD_MS);
        send_imu_sample(&sample, frame, sizeof(frame));
    }

#if !SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC
    if (time_elapsed(now_ms, g_last_magnetometer_ms, SENSOR_HUB_MAGNETOMETER_PERIOD_MS)) {
        magnetometer_sample_c_t sample;
        g_last_magnetometer_ms = now_ms;
        if (mmc5603_provider_read(&g_mmc5603, now_ms, &sample) == 0) {
            magnetometer_sample_filter_apply(&g_magnetometer_filter, &sample);
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
            magnetometer_sample_filter_reset(&g_magnetometer_filter);
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_MMC5603_READY;
            g_status_flags |= SENSOR_HUB_STATUS_MAGNETOMETER_ERROR;
        }
    }

    if (time_elapsed(now_ms, g_last_barometer_ms, SENSOR_HUB_BAROMETER_PERIOD_MS)) {
        barometer_sample_c_t sample;
        const int result = bmp390_provider_read(&g_bmp390, now_ms, &sample);
        g_last_barometer_ms = now_ms;
        if (result == 0) {
            barometer_sample_filter_apply(&g_barometer_filter, &sample);
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
            barometer_sample_filter_reset(&g_barometer_filter);
            g_status_flags &= (uint16_t)~SENSOR_HUB_STATUS_BMP390_READY;
            g_status_flags |= SENSOR_HUB_STATUS_BAROMETER_ERROR;
        }
    }
#endif

    board_uart_poll();
}
