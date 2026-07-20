#include "bsp/board_i2c.h"
#include "bsp/board_tick.h"
#include "sensors/mmc5603_provider_c.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {

constexpr std::uint8_t kAddress = 0x30U;
constexpr std::uint8_t kData = 0x00U;
constexpr std::uint8_t kStatus = 0x18U;
constexpr std::uint8_t kControl0 = 0x1BU;
constexpr std::uint8_t kProductId = 0x39U;
constexpr std::uint8_t kMeasurementDone = 0x40U;

std::uint32_t g_tickMs = 0U;
std::uint32_t g_measurementStartMs = 0U;
std::uint32_t g_statusReadCount = 0U;
std::array<std::uint8_t, 9> g_sampleBytes{};

void resetFakeDevice()
{
    g_tickMs = 0U;
    g_measurementStartMs = 0U;
    g_statusReadCount = 0U;
    g_sampleBytes = {0x80U, 0x00U, 0x80U, 0x00U, 0x80U,
                     0x00U, 0x00U, 0x00U, 0x00U};
}

void testMeasurementWaitIsBounded()
{
    mmc5603_provider_t provider{};
    magnetometer_sample_c_t sample{};

    resetFakeDevice();
    assert(mmc5603_provider_init(&provider) == 0);
    assert(provider.initialized == 1);
    g_statusReadCount = 0U;

    assert(mmc5603_provider_read(&provider, 1234U, &sample) == 0);
    assert(g_statusReadCount == 1U);
    assert(g_tickMs - g_measurementStartMs >= 7U);
    assert(sample.uptime_ms == 1234U);
}

} // namespace

extern "C" std::uint32_t board_get_tick_ms(void)
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
    if (registerAddress == kProductId && length == 1U) {
        data[0] = 0x10U;
        return 0;
    }
    if (registerAddress == kStatus && length == 1U) {
        ++g_statusReadCount;
        data[0] = g_tickMs - g_measurementStartMs >= 7U ? kMeasurementDone : 0U;
        return 0;
    }
    if (registerAddress == kData && length == g_sampleBytes.size()) {
        std::memcpy(data, g_sampleBytes.data(), g_sampleBytes.size());
        return 0;
    }
    return -1;
}

extern "C" int board_i2c_mem_write(std::uint8_t deviceAddress,
                                     std::uint8_t registerAddress,
                                     const std::uint8_t* data,
                                     std::size_t length)
{
    if (deviceAddress != kAddress || data == nullptr || length != 1U) {
        return -1;
    }
    if (registerAddress == kControl0 && data[0] == 0x01U) {
        g_measurementStartMs = g_tickMs;
    }
    return 0;
}

int main()
{
    testMeasurementWaitIsBounded();
    std::cout << "MMC5603 provider tests passed\n";
    return 0;
}
