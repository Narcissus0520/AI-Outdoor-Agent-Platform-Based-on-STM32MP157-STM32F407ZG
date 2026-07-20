#include "bsp/uart_tx_queue_c.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

int uart_tx_queue_init(uart_tx_queue_c_t* queue, uint8_t* storage, uint16_t capacity)
{
    if (queue == 0 || storage == 0 || capacity < 2U) {
        return -1;
    }

    queue->storage = storage;
    queue->capacity = capacity;
    queue->head = 0U;
    queue->tail = 0U;
    return 0;
}

size_t uart_tx_queue_pending(const uart_tx_queue_c_t* queue)
{
    const uint16_t head = queue->head;
    const uint16_t tail = queue->tail;

    if (head >= tail) {
        return (size_t)(head - tail);
    }
    return (size_t)queue->capacity - (size_t)tail + (size_t)head;
}

size_t uart_tx_queue_free(const uart_tx_queue_c_t* queue)
{
    return (size_t)queue->capacity - uart_tx_queue_pending(queue) - 1U;
}

int uart_tx_queue_enqueue(uart_tx_queue_c_t* queue, const uint8_t* data, size_t length)
{
    uint16_t head;
    size_t first_length;

    if (queue == 0 || queue->storage == 0 || data == 0 || length == 0U ||
        length > uart_tx_queue_free(queue)) {
        return -1;
    }

    head = queue->head;
    first_length = (size_t)queue->capacity - (size_t)head;
    if (first_length > length) {
        first_length = length;
    }
    (void)memcpy(&queue->storage[head], data, first_length);
    if (length > first_length) {
        (void)memcpy(queue->storage, &data[first_length], length - first_length);
    }

    queue->head = (uint16_t)(((size_t)head + length) % (size_t)queue->capacity);
    return 0;
}

size_t uart_tx_queue_peek_contiguous(const uart_tx_queue_c_t* queue,
                                     const uint8_t** data)
{
    const uint16_t head = queue->head;
    const uint16_t tail = queue->tail;

    if (data == 0 || head == tail) {
        return 0U;
    }

    *data = &queue->storage[tail];
    if (head > tail) {
        return (size_t)(head - tail);
    }
    return (size_t)queue->capacity - (size_t)tail;
}

void uart_tx_queue_consume(uart_tx_queue_c_t* queue, size_t length)
{
    const size_t pending = uart_tx_queue_pending(queue);

    if (length > pending) {
        length = pending;
    }
    queue->tail = (uint16_t)(((size_t)queue->tail + length) % (size_t)queue->capacity);
}
