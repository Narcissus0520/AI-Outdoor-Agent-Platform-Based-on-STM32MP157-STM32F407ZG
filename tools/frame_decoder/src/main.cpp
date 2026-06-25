#include "protocol/barometer_payload.h"
#include "protocol/imu_payload.h"
#include "protocol/magnetometer_payload.h"
#include "protocol/mcu_protocol.h"

#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int hexValue(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    return -1;
}

bool parseHex(const std::string& input, std::vector<std::uint8_t>& bytes, std::string& error)
{
    std::string compact;
    for (char ch : input) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == ':' || ch == '-') {
            continue;
        }
        compact.push_back(ch);
    }

    if (compact.empty()) {
        error = "empty frame";
        return false;
    }

    if ((compact.size() % 2U) != 0U) {
        error = "hex input has odd number of digits";
        return false;
    }

    bytes.clear();
    for (std::size_t index = 0; index < compact.size(); index += 2U) {
        const int high = hexValue(compact[index]);
        const int low = hexValue(compact[index + 1U]);
        if (high < 0 || low < 0) {
            error = "hex input contains non-hex characters";
            bytes.clear();
            return false;
        }
        bytes.push_back(static_cast<std::uint8_t>((high << 4U) | low));
    }
    return true;
}

std::uint16_t readU16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::uint32_t readU32Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::int16_t readI16Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::int16_t>(readU16Le(bytes, offset));
}

std::int32_t readI32Le(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U));
}

const char* typeName(std::uint8_t type)
{
    switch (type) {
    case outdoor::protocol::MSG_TYPE_HEARTBEAT:
        return "heartbeat";
    case outdoor::protocol::MSG_TYPE_MOCK_SENSOR:
        return "mock_sensor";
    case outdoor::protocol::MSG_TYPE_SENSOR_IMU:
        return "sensor_imu";
    case outdoor::protocol::MSG_TYPE_SENSOR_MAGNETOMETER:
        return "sensor_magnetometer";
    case outdoor::protocol::MSG_TYPE_SENSOR_BAROMETER:
        return "sensor_barometer";
    default:
        return "unknown";
    }
}

void printImuPayload(const std::vector<std::uint8_t>& payload)
{
    if (payload.size() != outdoor::protocol::kImuPayloadSize) {
        std::cout << "imu_payload_error=invalid_size\n";
        return;
    }

    const outdoor::protocol::ImuSample sample {
        readU32Le(payload, 0),
        readI16Le(payload, 4),
        readI16Le(payload, 6),
        readI16Le(payload, 8),
        readI32Le(payload, 10),
        readI32Le(payload, 14),
        readI32Le(payload, 18),
        readI16Le(payload, 22),
    };

    std::cout << std::fixed << std::setprecision(3)
              << "imu.uptime_ms=" << sample.uptimeMs << "\n"
              << "imu.accel_g=("
              << outdoor::protocol::accelMgToG(sample.accelXMg) << ", "
              << outdoor::protocol::accelMgToG(sample.accelYMg) << ", "
              << outdoor::protocol::accelMgToG(sample.accelZMg) << ")\n"
              << "imu.gyro_dps=("
              << outdoor::protocol::gyroMdpsToDps(sample.gyroXMdps) << ", "
              << outdoor::protocol::gyroMdpsToDps(sample.gyroYMdps) << ", "
              << outdoor::protocol::gyroMdpsToDps(sample.gyroZMdps) << ")\n"
              << "imu.temperature_celsius="
              << outdoor::protocol::centiCelsiusToCelsius(sample.temperatureCentiC) << "\n";
}

void printMagnetometerPayload(const std::vector<std::uint8_t>& payload)
{
    if (payload.size() != outdoor::protocol::kMagnetometerPayloadSize) {
        std::cout << "magnetometer_payload_error=invalid_size\n";
        return;
    }

    const auto x = readI32Le(payload, 4);
    const auto y = readI32Le(payload, 8);
    const auto z = readI32Le(payload, 12);
    std::cout << std::fixed << std::setprecision(3)
              << "magnetometer.uptime_ms=" << readU32Le(payload, 0) << "\n"
              << "magnetometer.microtesla=("
              << outdoor::protocol::nanoTeslaToMicroTesla(x) << ", "
              << outdoor::protocol::nanoTeslaToMicroTesla(y) << ", "
              << outdoor::protocol::nanoTeslaToMicroTesla(z) << ")\n";
}

void printBarometerPayload(const std::vector<std::uint8_t>& payload)
{
    if (payload.size() != outdoor::protocol::kBarometerPayloadSize) {
        std::cout << "barometer_payload_error=invalid_size\n";
        return;
    }

    std::cout << std::fixed << std::setprecision(2)
              << "barometer.uptime_ms=" << readU32Le(payload, 0) << "\n"
              << "barometer.pressure_pa=" << readU32Le(payload, 4) << "\n"
              << "barometer.temperature_celsius="
              << outdoor::protocol::centiCelsiusToCelsiusBarometer(
                     readI16Le(payload, 8)) << "\n";
}

} // namespace

int main(int argc, char* argv[])
{
    std::ostringstream input;
    if (argc > 1) {
        for (int index = 1; index < argc; ++index) {
            input << argv[index];
        }
    } else {
        input << std::cin.rdbuf();
    }

    std::vector<std::uint8_t> bytes;
    std::string error;
    if (!parseHex(input.str(), bytes, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    if (bytes.size() < outdoor::protocol::kFrameHeaderSize + outdoor::protocol::kFrameCrcSize) {
        std::cerr << "frame is too short\n";
        return 1;
    }

    if (bytes[0] != outdoor::protocol::kFrameSof0 || bytes[1] != outdoor::protocol::kFrameSof1) {
        std::cerr << "invalid SOF\n";
        return 1;
    }

    const std::uint16_t payloadSize = readU16Le(bytes, 6);
    const std::size_t expectedSize = outdoor::protocol::kFrameHeaderSize +
                                     payloadSize +
                                     outdoor::protocol::kFrameCrcSize;
    if (bytes.size() != expectedSize) {
        std::cerr << "frame length does not match payload size\n";
        return 1;
    }

    const std::uint16_t expectedCrc = readU16Le(bytes, outdoor::protocol::kFrameHeaderSize + payloadSize);
    const std::uint16_t actualCrc = outdoor::protocol::crc16Modbus(
        bytes.data() + 2,
        outdoor::protocol::kFrameHeaderSize - 2 + payloadSize);

    const std::uint8_t type = bytes[3];
    std::cout << "version=" << static_cast<int>(bytes[2]) << "\n"
              << "type=0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(type) << std::dec << std::setfill(' ')
              << " (" << typeName(type) << ")\n"
              << "sequence=" << readU16Le(bytes, 4) << "\n"
              << "payload_length=" << payloadSize << "\n"
              << "crc_expected=0x" << std::hex << std::setw(4) << std::setfill('0') << expectedCrc << "\n"
              << "crc_calculated=0x" << std::setw(4) << actualCrc << std::dec << std::setfill(' ') << "\n"
              << "crc_ok=" << (expectedCrc == actualCrc ? "true" : "false") << "\n";

    const std::vector<std::uint8_t> payload(
        bytes.begin() + static_cast<std::ptrdiff_t>(outdoor::protocol::kFrameHeaderSize),
        bytes.begin() + static_cast<std::ptrdiff_t>(outdoor::protocol::kFrameHeaderSize + payloadSize));

    if (type == outdoor::protocol::MSG_TYPE_SENSOR_IMU) {
        printImuPayload(payload);
    } else if (type == outdoor::protocol::MSG_TYPE_SENSOR_MAGNETOMETER) {
        printMagnetometerPayload(payload);
    } else if (type == outdoor::protocol::MSG_TYPE_SENSOR_BAROMETER) {
        printBarometerPayload(payload);
    }

    return expectedCrc == actualCrc ? 0 : 1;
}
