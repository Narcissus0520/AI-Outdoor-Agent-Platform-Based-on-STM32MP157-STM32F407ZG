#include "bsp/board_uart.h"

#include "bsp/uart_tx_queue_c.h"
#include "usart.h"

#include <stddef.h>
#include <stdint.h>

enum {
    BOARD_UART_RX_BUFFER_SIZE = 64U,
    BOARD_UART_TX_BUFFER_SIZE = 1024U,
};

typedef struct {
    UART_HandleTypeDef* uart;
    uart_tx_queue_c_t queue;
    uint8_t storage[BOARD_UART_TX_BUFFER_SIZE];
    volatile uint16_t active_length;
    uint32_t dropped_frame_count;
} board_uart_tx_channel_t;

static uint8_t g_uart4_rx_buffer[BOARD_UART_RX_BUFFER_SIZE];
static uint8_t g_uart4_rx_byte;
static volatile uint16_t g_uart4_rx_head;
static volatile uint16_t g_uart4_rx_tail;
static volatile uint32_t g_uart4_rx_drop_count;
static board_uart_tx_channel_t g_uart4_tx;
static board_uart_tx_channel_t g_usart1_tx;

static void start_uart4_receive(void)
{
    (void)HAL_UART_Receive_IT(&huart4, &g_uart4_rx_byte, 1U);
}

static void init_tx_channel(board_uart_tx_channel_t* channel, UART_HandleTypeDef* uart)
{
    channel->uart = uart;
    channel->active_length = 0U;
    channel->dropped_frame_count = 0U;
    (void)uart_tx_queue_init(&channel->queue,
                             channel->storage,
                             (uint16_t)sizeof(channel->storage));
}

static void kick_tx_channel(board_uart_tx_channel_t* channel)
{
    const uint8_t* data = 0;
    size_t length;

    if (channel->active_length != 0U) {
        return;
    }

    length = uart_tx_queue_peek_contiguous(&channel->queue, &data);
    if (length == 0U) {
        return;
    }

    channel->active_length = (uint16_t)length;
    if (HAL_UART_Transmit_IT(channel->uart,
                            (uint8_t*)data,
                            channel->active_length) != HAL_OK) {
        channel->active_length = 0U;
    }
}

static int enqueue_tx_frame(board_uart_tx_channel_t* channel,
                            const uint8_t* data,
                            size_t length)
{
    if (uart_tx_queue_enqueue(&channel->queue, data, length) != 0) {
        ++channel->dropped_frame_count;
        return -1;
    }
    kick_tx_channel(channel);
    return 0;
}

int board_uart_init(void)
{
    g_uart4_rx_head = 0U;
    g_uart4_rx_tail = 0U;
    g_uart4_rx_drop_count = 0U;
    init_tx_channel(&g_uart4_tx, &huart4);
    init_tx_channel(&g_usart1_tx, &huart1);

    __HAL_UART_CLEAR_OREFLAG(&huart4);
    HAL_NVIC_SetPriority(UART4_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    if (HAL_UART_Receive_IT(&huart4, &g_uart4_rx_byte, 1U) != HAL_OK) {
        return -1;
    }
    return 0;
}

int board_uart_send_bytes(const uint8_t* data, size_t length)
{
    int primary_result;

    if (data == 0 || length == 0U || length >= BOARD_UART_TX_BUFFER_SIZE) {
        return -1;
    }

    primary_result = enqueue_tx_frame(&g_uart4_tx, data, length);
    (void)enqueue_tx_frame(&g_usart1_tx, data, length);
    return primary_result;
}

int board_uart_send_diagnostic_bytes(const uint8_t* data, size_t length)
{
    if (data == 0 || length == 0U || length >= BOARD_UART_TX_BUFFER_SIZE) {
        return -1;
    }

    return enqueue_tx_frame(&g_usart1_tx, data, length);
}

int board_uart_receive_byte(uint8_t* out_byte)
{
    uint16_t tail;

    if (out_byte == 0) {
        return -1;
    }

    tail = g_uart4_rx_tail;
    if (tail == g_uart4_rx_head) {
        return 0;
    }

    *out_byte = g_uart4_rx_buffer[tail];
    g_uart4_rx_tail = (uint16_t)((tail + 1U) % BOARD_UART_RX_BUFFER_SIZE);
    return 1;
}

void board_uart_poll(void)
{
    kick_tx_channel(&g_uart4_tx);
    kick_tx_channel(&g_usart1_tx);
}

uint32_t board_uart_primary_tx_drop_count(void)
{
    return g_uart4_tx.dropped_frame_count;
}

uint32_t board_uart_diagnostic_tx_drop_count(void)
{
    return g_usart1_tx.dropped_frame_count;
}

uint32_t board_uart_rx_drop_count(void)
{
    return g_uart4_rx_drop_count;
}

size_t board_uart_primary_tx_pending_bytes(void)
{
    return uart_tx_queue_pending(&g_uart4_tx.queue);
}

size_t board_uart_diagnostic_tx_pending_bytes(void)
{
    return uart_tx_queue_pending(&g_usart1_tx.queue);
}

void UART4_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart4);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    board_uart_tx_channel_t* channel = 0;
    uint16_t completed_length;

    if (huart == &huart4) {
        channel = &g_uart4_tx;
    } else if (huart == &huart1) {
        channel = &g_usart1_tx;
    }
    if (channel == 0) {
        return;
    }

    completed_length = channel->active_length;
    channel->active_length = 0U;
    uart_tx_queue_consume(&channel->queue, completed_length);
    kick_tx_channel(channel);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart4) {
        const uint16_t head = g_uart4_rx_head;
        const uint16_t next = (uint16_t)((head + 1U) % BOARD_UART_RX_BUFFER_SIZE);

        if (next != g_uart4_rx_tail) {
            g_uart4_rx_buffer[head] = g_uart4_rx_byte;
            g_uart4_rx_head = next;
        } else {
            ++g_uart4_rx_drop_count;
        }
        start_uart4_receive();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart4) {
        __HAL_UART_CLEAR_OREFLAG(&huart4);
        start_uart4_receive();
    }
}
