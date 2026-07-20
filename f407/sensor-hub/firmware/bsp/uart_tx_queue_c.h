#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t* storage;
    uint16_t capacity;
    volatile uint16_t head;
    volatile uint16_t tail;
} uart_tx_queue_c_t;

int uart_tx_queue_init(uart_tx_queue_c_t* queue, uint8_t* storage, uint16_t capacity);
int uart_tx_queue_enqueue(uart_tx_queue_c_t* queue, const uint8_t* data, size_t length);
size_t uart_tx_queue_pending(const uart_tx_queue_c_t* queue);
size_t uart_tx_queue_free(const uart_tx_queue_c_t* queue);
size_t uart_tx_queue_peek_contiguous(const uart_tx_queue_c_t* queue,
                                     const uint8_t** data);
void uart_tx_queue_consume(uart_tx_queue_c_t* queue, size_t length);

#ifdef __cplusplus
}
#endif
