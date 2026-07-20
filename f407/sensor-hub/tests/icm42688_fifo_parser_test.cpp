#include "sensors/icm42688_fifo_parser_c.h"
#include "sensors/imu_timestamp_normalizer_c.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

void writeI16Be(std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE>& packet,
                std::size_t offset,
                std::int16_t value)
{
    const auto bits = static_cast<std::uint16_t>(value);
    packet[offset] = static_cast<std::uint8_t>(bits >> 8U);
    packet[offset + 1U] = static_cast<std::uint8_t>(bits & 0xFFU);
}

std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE> makeValidPacket()
{
    std::array<std::uint8_t, ICM42688_FIFO_PACKET_SIZE> packet{};
    packet[0] = 0x68U;
    writeI16Be(packet, 1U, 8192);
    writeI16Be(packet, 3U, -4096);
    writeI16Be(packet, 5U, 2048);
    writeI16Be(packet, 7U, 328);
    writeI16Be(packet, 9U, -164);
    writeI16Be(packet, 11U, 0);
    packet[13] = 2U;
    packet[14] = 0x12U;
    packet[15] = 0x34U;
    return packet;
}

void testValidPacket()
{
    const auto packet = makeValidPacket();
    imu_sample_c_t sample{};

    assert(icm42688_fifo_parse_packet(packet.data(), 1234U, &sample) == 0);
    assert(sample.uptime_ms == 1234U);
    assert(sample.accel_x_mg == 1000);
    assert(sample.accel_y_mg == -500);
    assert(sample.accel_z_mg == 250);
    assert(sample.gyro_x_mdps == 10009);
    assert(sample.gyro_y_mdps == -5004);
    assert(sample.gyro_z_mdps == 0);
    assert(sample.temperature_centi_c == 2596);
}

void testHeaderAndInvalidDataRejection()
{
    auto packet = makeValidPacket();
    imu_sample_c_t sample{};

    packet[0] = 0x80U;
    assert(icm42688_fifo_parse_packet(packet.data(), 0U, &sample) ==
           ICM42688_FIFO_PARSE_EMPTY_MESSAGE);

    packet[0] = 0x60U;
    assert(icm42688_fifo_parse_packet(packet.data(), 0U, &sample) ==
           ICM42688_FIFO_PARSE_HEADER_FORMAT);

    packet = makeValidPacket();
    writeI16Be(packet, 9U, INT16_MIN);
    assert(icm42688_fifo_parse_packet(packet.data(), 0U, &sample) ==
           ICM42688_FIFO_PARSE_INVALID_SAMPLE);
    assert(icm42688_fifo_parse_packet(nullptr, 0U, &sample) != 0);
    assert(icm42688_fifo_parse_packet(packet.data(), 0U, nullptr) != 0);
}

void testVariablePacketStream()
{
    std::array<std::uint8_t, 33> stream{};
    const auto packet = makeValidPacket();
    std::array<imu_sample_c_t, 2> samples{};
    std::size_t sampleCount = 0U;
    std::size_t skippedCount = 0U;

    stream[0] = 0x40U;
    for (std::size_t index = 0U; index < packet.size(); ++index) {
        stream[8U + index] = packet[index];
    }
    stream[24] = 0x20U;
    stream[32] = 0x80U;

    assert(icm42688_fifo_parse_stream(stream.data(),
                                      stream.size(),
                                      500U,
                                      10U,
                                      samples.data(),
                                      samples.size(),
                                      &sampleCount,
                                      &skippedCount) == 0);
    assert(sampleCount == 1U);
    assert(skippedCount == 3U);
    assert(samples[0].uptime_ms == 490U);
    assert(samples[0].accel_x_mg == 1000);

    assert(icm42688_fifo_parse_stream(packet.data(),
                                      packet.size() - 1U,
                                      0U,
                                      10U,
                                      samples.data(),
                                      samples.size(),
                                      &sampleCount,
                                      &skippedCount) == ICM42688_FIFO_PARSE_TRUNCATED);
}

void testEmptyMarkerStopsParsingFillerBytes()
{
    std::array<std::uint8_t, 32> stream{};
    const auto packet = makeValidPacket();
    std::array<imu_sample_c_t, 2> samples{};
    std::size_t sampleCount = 0U;
    std::size_t skippedCount = 0U;

    std::copy(packet.begin(), packet.end(), stream.begin());
    std::fill(stream.begin() + 16, stream.end(), 0x80U);

    assert(icm42688_fifo_parse_stream(stream.data(),
                                      stream.size(),
                                      500U,
                                      10U,
                                      samples.data(),
                                      samples.size(),
                                      &sampleCount,
                                      &skippedCount) == 0);
    assert(sampleCount == 1U);
    assert(skippedCount == 1U);
    assert(samples[0].uptime_ms == 500U);
}

void testSkippedPacketsRemainOnTimeline()
{
    std::array<std::uint8_t, 40> stream{};
    auto firstPacket = makeValidPacket();
    auto secondPacket = makeValidPacket();
    std::array<imu_sample_c_t, 2> samples{};
    std::size_t sampleCount = 0U;
    std::size_t skippedCount = 0U;

    firstPacket[0] = 0x6BU; // ODR-change flags do not change the packet layout.
    secondPacket[0] = 0x68U;
    std::copy(firstPacket.begin(), firstPacket.end(), stream.begin());
    stream[16] = 0x40U;
    std::copy(secondPacket.begin(), secondPacket.end(), stream.begin() + 24);

    assert(icm42688_fifo_parse_stream(stream.data(),
                                      stream.size(),
                                      500U,
                                      10U,
                                      samples.data(),
                                      samples.size(),
                                      &sampleCount,
                                      &skippedCount) == 0);
    assert(sampleCount == 2U);
    assert(skippedCount == 1U);
    assert(samples[0].uptime_ms == 480U);
    assert(samples[1].uptime_ms == 500U);
}

void testOverlappingBatchesKeepTheirSampleSpacing()
{
    imu_timestamp_normalizer_c_t normalizer{};
    std::array<imu_sample_c_t, 3> firstBatch{};
    std::array<imu_sample_c_t, 3> overlappingBatch{};
    imu_sample_c_t fallbackSample{};

    firstBatch[0].uptime_ms = 100U;
    firstBatch[1].uptime_ms = 110U;
    firstBatch[2].uptime_ms = 120U;
    overlappingBatch[0].uptime_ms = 115U;
    overlappingBatch[1].uptime_ms = 125U;
    overlappingBatch[2].uptime_ms = 135U;
    fallbackSample.uptime_ms = 140U;

    imu_timestamp_normalizer_init(&normalizer);
    assert(imu_timestamp_normalizer_apply(&normalizer,
                                          firstBatch.data(),
                                          firstBatch.size(),
                                          10U) == 0);
    assert(imu_timestamp_normalizer_apply(&normalizer,
                                          overlappingBatch.data(),
                                          overlappingBatch.size(),
                                          10U) == 0);
    assert(overlappingBatch[0].uptime_ms == 130U);
    assert(overlappingBatch[1].uptime_ms == 140U);
    assert(overlappingBatch[2].uptime_ms == 150U);

    assert(imu_timestamp_normalizer_apply(&normalizer,
                                          &fallbackSample,
                                          1U,
                                          10U) == 0);
    assert(fallbackSample.uptime_ms == 160U);
}

void testNewBatchContinuesThePublishedSampleTimeline()
{
    imu_timestamp_normalizer_c_t normalizer{};
    std::array<imu_sample_c_t, 2> firstBatch{};
    std::array<imu_sample_c_t, 2> nextBatch{};

    firstBatch[0].uptime_ms = 100U;
    firstBatch[1].uptime_ms = 110U;
    nextBatch[0].uptime_ms = 130U;
    nextBatch[1].uptime_ms = 140U;

    imu_timestamp_normalizer_init(&normalizer);
    assert(imu_timestamp_normalizer_apply(&normalizer,
                                          firstBatch.data(),
                                          firstBatch.size(),
                                          10U) == 0);
    assert(imu_timestamp_normalizer_apply(&normalizer,
                                          nextBatch.data(),
                                          nextBatch.size(),
                                          10U) == 0);
    assert(nextBatch[0].uptime_ms == 120U);
    assert(nextBatch[1].uptime_ms == 130U);
}

} // namespace

int main()
{
    testValidPacket();
    testHeaderAndInvalidDataRejection();
    testVariablePacketStream();
    testEmptyMarkerStopsParsingFillerBytes();
    testSkippedPacketsRemainOnTimeline();
    testOverlappingBatchesKeepTheirSampleSpacing();
    testNewBatchContinuesThePublishedSampleTimeline();
    std::cout << "ICM42688 FIFO parser tests passed\n";
    return 0;
}
