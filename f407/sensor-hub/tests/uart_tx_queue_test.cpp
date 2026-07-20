#include "bsp/uart_tx_queue_c.h"

#include <array>
#include "test_check.h"
#include <cstdint>
#include <iostream>

namespace {

void testQueueWrapAndContiguousSegments()
{
    std::array<std::uint8_t, 8> storage{};
    uart_tx_queue_c_t queue{};
    const std::array<std::uint8_t, 5> first{1, 2, 3, 4, 5};
    const std::array<std::uint8_t, 4> second{6, 7, 8, 9};
    const std::uint8_t* data = nullptr;

    assert(uart_tx_queue_init(&queue, storage.data(), storage.size()) == 0);
    assert(uart_tx_queue_pending(&queue) == 0U);
    assert(uart_tx_queue_free(&queue) == 7U);
    assert(uart_tx_queue_enqueue(&queue, first.data(), first.size()) == 0);
    assert(uart_tx_queue_peek_contiguous(&queue, &data) == 5U);
    assert(data[0] == 1U && data[4] == 5U);

    uart_tx_queue_consume(&queue, 3U);
    assert(uart_tx_queue_enqueue(&queue, second.data(), second.size()) == 0);
    assert(uart_tx_queue_pending(&queue) == 6U);
    assert(uart_tx_queue_peek_contiguous(&queue, &data) == 5U);
    assert(data[0] == 4U && data[1] == 5U && data[2] == 6U && data[4] == 8U);

    uart_tx_queue_consume(&queue, 5U);
    assert(uart_tx_queue_peek_contiguous(&queue, &data) == 1U);
    assert(data[0] == 9U);
    uart_tx_queue_consume(&queue, 1U);
    assert(uart_tx_queue_pending(&queue) == 0U);
}

void testFrameRejectionIsAtomic()
{
    std::array<std::uint8_t, 8> storage{};
    uart_tx_queue_c_t queue{};
    const std::array<std::uint8_t, 6> first{1, 2, 3, 4, 5, 6};
    const std::array<std::uint8_t, 2> rejected{7, 8};
    const std::uint8_t* data = nullptr;

    assert(uart_tx_queue_init(&queue, storage.data(), storage.size()) == 0);
    assert(uart_tx_queue_enqueue(&queue, first.data(), first.size()) == 0);
    assert(uart_tx_queue_enqueue(&queue, rejected.data(), rejected.size()) != 0);
    assert(uart_tx_queue_pending(&queue) == first.size());
    assert(uart_tx_queue_peek_contiguous(&queue, &data) == first.size());
    assert(data[5] == 6U);
}

} // namespace

int main()
{
    testQueueWrapAndContiguousSegments();
    testFrameRejectionIsAtomic();
    std::cout << "UART TX queue tests passed\n";
    return 0;
}
