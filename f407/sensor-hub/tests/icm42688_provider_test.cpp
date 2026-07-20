#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"
#include "sensors/icm42688_fifo_parser_c.h"
#include "sensors/icm42688_provider_c.h"

#include <array>
#include "test_check.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

constexpr std::uint8_t kAddress = 0x69U;
constexpr std::uint8_t kDeviceConfig = 0x11U;
constexpr std::uint8_t kFifoConfig = 0x16U;
constexpr std::uint8_t kFifoCountHigh = 0x2EU;
constexpr std::uint8_t kFifoData = 0x30U;
constexpr std::uint8_t kSignalPathReset = 0x4BU;
constexpr std::uint8_t kInterfaceConfig0 = 0x4CU;
constexpr std::uint8_t kPowerManagement0 = 0x4EU;
constexpr std::uint8_t kFifoConfig1 = 0x5FU;
constexpr std::uint8_t kFifoConfig2 = 0x60U;
constexpr std::uint8_t kIntSource0 = 0x65U;
constexpr std::uint8_t kWhoAmI = 0x75U;

std::array<std::uint8_t, 256> g_registers{};
std::vector<std::uint8_t> g_fifo;
std::uint32_t g_tickMs = 0U;
std::uint32_t g_fifoFlushCount = 0U;
std::size_t g_maxFifoReadLength = 0U;
bool g_replenishFifoDuringRead = false;
bool g_failNextNonReplayableRead = false;

void resetFakeDevice()
{
    g_registers.fill(0U);
    g_registers[kWhoAmI] = 0x47U;
    g_fifo.clear();
    g_tickMs = 0U;
    g_fifoFlushCount = 0U;
    g_maxFifoReadLength = 0U;
    g_replenishFifoDuringRead = false;
    g_failNextNonReplayableRead = false;
}

void writeI16Be(std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE>& packet,
                std::size_t offset,
                std::int16_t value)
{
    const auto bits = static_cast<std::uint16_t>(value);
    packet[offset] = static_cast<std::uint8_t>(bits >> 8U);
    packet[offset + 1U] = static_cast<std::uint8_t>(bits & 0xFFU);
}

std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE> makePacket(std::int16_t accelX)
{
    std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE> packet{};
    packet[0] = 0x68U;
    writeI16Be(packet, 1U, accelX);
    writeI16Be(packet, 3U, 0);
    writeI16Be(packet, 5U, 8192);
    writeI16Be(packet, 7U, 0);
    writeI16Be(packet, 9U, 0);
    writeI16Be(packet, 11U, 0);
    packet[13] = 0U;
    return packet;
}

void appendPacket(const std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE>& packet)
{
    g_fifo.insert(g_fifo.end(), packet.begin(), packet.end());
}

icm42688_provider_t initializeProvider()
{
    icm42688_provider_t provider{};
    icm42688_provider_reset(&provider);
    assert(icm42688_provider_init(&provider) == 0);
    assert(provider.initialized == 1);
    assert(provider.i2c_address == kAddress);
    assert(provider.init_error_step == 0U);
    return provider;
}

void testReinitializationPreservesDiagnostics()
{
    resetFakeDevice();
    auto provider = initializeProvider();
    provider.fifo_overflow_count = 7U;
    provider.fifo_malformed_packet_count = 5U;
    provider.fifo_empty_event_count = 3U;

    assert(icm42688_provider_init(&provider) == 0);
    assert(provider.fifo_overflow_count == 7U);
    assert(provider.fifo_malformed_packet_count == 5U);
    assert(provider.fifo_empty_event_count == 3U);
}

void testInitializationProgramsFifoAndInterrupts()
{
    resetFakeDevice();
    const auto provider = initializeProvider();

    (void)provider;
    assert(g_registers[kDeviceConfig] == 0x01U);
    assert((g_registers[kInterfaceConfig0] & 0x70U) == 0x70U);
    assert(g_registers[kFifoConfig1] == 0x27U);
    assert(g_registers[kFifoConfig2] == 4U);
    assert((g_registers[kPowerManagement0] & 0x3FU) == 0x0FU);
    assert((g_registers[kFifoConfig] & 0xC0U) == 0x40U);
    assert(g_registers[kIntSource0] == 0x06U);
    assert(g_fifoFlushCount == 1U);
}

void testCombinedBatchReadAndTimestamps()
{
    std::array<imu_sample_c_t, ICM42688_FIFO_BATCH_CAPACITY> samples{};
    std::size_t sampleCount = 0U;

    resetFakeDevice();
    auto provider = initializeProvider();
    appendPacket(makePacket(1024));
    appendPacket(makePacket(2048));
    appendPacket(makePacket(3072));
    appendPacket(makePacket(4096));

    assert(icm42688_provider_read_fifo(&provider,
                                       1000U,
                                       samples.data(),
                                       samples.size(),
                                       &sampleCount) == 0);
    assert(sampleCount == 4U);
    assert(samples[0].uptime_ms == 970U);
    assert(samples[1].uptime_ms == 980U);
    assert(samples[2].uptime_ms == 990U);
    assert(samples[3].uptime_ms == 1000U);
    assert(samples[0].accel_x_mg == 125);
    assert(samples[3].accel_x_mg == 500);
    assert(g_fifo.empty());
    assert(g_maxFifoReadLength == 4U * ICM42688_FIFO_PACKET_SIZE);
}

void testOversizedFifoIsFlushed()
{
    std::array<imu_sample_c_t, ICM42688_FIFO_BATCH_CAPACITY> samples{};
    std::size_t sampleCount = 0U;

    resetFakeDevice();
    auto provider = initializeProvider();
    for (std::size_t index = 0U; index < 17U; ++index) {
        appendPacket(makePacket(static_cast<std::int16_t>(index)));
    }
    assert(icm42688_provider_read_fifo(&provider,
                                       100U,
                                       samples.data(),
                                       samples.size(),
                                       &sampleCount) == ICM42688_PROVIDER_FIFO_OVERFLOW);
    assert(sampleCount == 0U);
    assert(provider.fifo_overflow_count == 1U);
    assert(g_fifo.empty());
    assert(g_fifoFlushCount == 2U);
    assert(g_maxFifoReadLength == 0U);
}

void testMalformedPacketsAreFlushedAndConcurrentGrowthIsAllowed()
{
    std::array<imu_sample_c_t, ICM42688_FIFO_BATCH_CAPACITY> samples{};
    std::size_t sampleCount = 0U;

    resetFakeDevice();
    auto provider = initializeProvider();
    auto malformedPacket = makePacket(1);
    malformedPacket[0] = 0x10U;
    appendPacket(malformedPacket);
    assert(icm42688_provider_read_fifo(&provider,
                                       100U,
                                       samples.data(),
                                       samples.size(),
                                       &sampleCount) == ICM42688_PROVIDER_FIFO_HEADER_FORMAT);
    assert(provider.fifo_malformed_packet_count == 1U);
    assert(g_fifo.empty());

    resetFakeDevice();
    provider = initializeProvider();
    for (std::size_t index = 0U; index < 4U; ++index) {
        appendPacket(makePacket(static_cast<std::int16_t>(index)));
    }
    g_replenishFifoDuringRead = true;
    assert(icm42688_provider_read_fifo(&provider,
                                       100U,
                                       samples.data(),
                                       samples.size(),
                                       &sampleCount) == 0);
    assert(sampleCount == 4U);
    assert(provider.fifo_drain_stall_count == 0U);
    assert(g_fifo.size() == 4U * ICM42688_FIFO_PACKET_SIZE);
}

void testSpuriousInterruptIsCountedWithoutReadingFifo()
{
    std::array<imu_sample_c_t, ICM42688_FIFO_BATCH_CAPACITY> samples{};
    std::size_t sampleCount = 99U;

    resetFakeDevice();
    auto provider = initializeProvider();
    assert(icm42688_provider_read_fifo(&provider,
                                       100U,
                                       samples.data(),
                                       samples.size(),
                                       &sampleCount) == 0);
    assert(sampleCount == 0U);
    assert(provider.fifo_empty_event_count == 1U);
    assert(g_fifo.empty());
}

void testFailedFifoStreamReadIsFlushedWithoutReplay()
{
    std::array<imu_sample_c_t, ICM42688_FIFO_BATCH_CAPACITY> samples{};
    std::size_t sampleCount = 99U;

    resetFakeDevice();
    auto provider = initializeProvider();
    for (std::size_t index = 0U; index < 4U; ++index) {
        appendPacket(makePacket(static_cast<std::int16_t>(index)));
    }
    g_failNextNonReplayableRead = true;

    assert(icm42688_provider_read_fifo(&provider,
                                       100U,
                                       samples.data(),
                                       samples.size(),
                                       &sampleCount) == -1);
    assert(sampleCount == 0U);
    assert(g_fifo.empty());
    assert(g_fifoFlushCount == 2U);
    assert(g_maxFifoReadLength == 0U);
}

} // namespace

extern "C" uint32_t board_get_tick_ms(void)
{
    return g_tickMs++;
}

extern "C" int board_i2c_mem_read(std::uint8_t deviceAddress,
                                    std::uint8_t registerAddress,
                                    std::uint8_t* data,
                                    std::size_t length)
{
    if (deviceAddress != kAddress || data == nullptr || length == 0U) {
        return -1;
    }

    if (registerAddress == kFifoData) {
        if (length > g_fifo.size()) {
            return -1;
        }
        std::memcpy(data, g_fifo.data(), length);
        if (length > g_maxFifoReadLength) {
            g_maxFifoReadLength = length;
        }
        g_fifo.erase(g_fifo.begin(),
                     g_fifo.begin() + static_cast<std::ptrdiff_t>(length));
        return 0;
    }
    if (registerAddress == kFifoCountHigh && length == 2U) {
        const std::size_t records = g_fifo.size() / ICM42688_FIFO_PACKET_SIZE;
        data[0] = static_cast<std::uint8_t>((records >> 8U) & 0xFFU);
        data[1] = static_cast<std::uint8_t>(records & 0xFFU);
        if (g_replenishFifoDuringRead) {
            for (std::size_t index = 0U; index < 4U; ++index) {
                appendPacket(makePacket(static_cast<std::int16_t>(100 + index)));
            }
            g_replenishFifoDuringRead = false;
        }
        return 0;
    }
    if (static_cast<std::size_t>(registerAddress) + length > g_registers.size()) {
        return -1;
    }
    std::memcpy(data, &g_registers[registerAddress], length);
    return 0;
}

extern "C" int board_i2c_mem_read_non_replayable(std::uint8_t deviceAddress,
                                                   std::uint8_t registerAddress,
                                                   std::uint8_t* data,
                                                   std::size_t length)
{
    if (g_failNextNonReplayableRead) {
        g_failNextNonReplayableRead = false;
        return -1;
    }
    return board_i2c_mem_read(deviceAddress, registerAddress, data, length);
}

extern "C" int board_i2c_mem_write(std::uint8_t deviceAddress,
                                     std::uint8_t registerAddress,
                                     const std::uint8_t* data,
                                     std::size_t length)
{
    if (deviceAddress != kAddress || data == nullptr || length == 0U ||
        static_cast<std::size_t>(registerAddress) + length > g_registers.size()) {
        return -1;
    }

    std::memcpy(&g_registers[registerAddress], data, length);
    if (registerAddress == kSignalPathReset && (data[0] & 0x02U) != 0U) {
        g_fifo.clear();
        ++g_fifoFlushCount;
    }
    return 0;
}

int main()
{
    testInitializationProgramsFifoAndInterrupts();
    testCombinedBatchReadAndTimestamps();
    testOversizedFifoIsFlushed();
    testMalformedPacketsAreFlushedAndConcurrentGrowthIsAllowed();
    testSpuriousInterruptIsCountedWithoutReadingFifo();
    testFailedFifoStreamReadIsFlushedWithoutReplay();
    testReinitializationPreservesDiagnostics();
    std::cout << "ICM42688 provider tests passed\n";
    return 0;
}
