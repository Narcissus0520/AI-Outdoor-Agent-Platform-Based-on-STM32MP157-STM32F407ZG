#include "sensors/icm20608_char_reader.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace {

bool almostEqual(double left, double right)
{
    return std::fabs(left - right) < 0.0005;
}

} // namespace

int main()
{
    const auto path = std::filesystem::temp_directory_path() / "icm20608_char_reader_tests.bin";
    std::filesystem::remove(path);

    const std::array<std::int32_t, 7> raw {
        164,
        -82,
        0,
        2048,
        -1024,
        0,
        3293,
    };

    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        assert(stream.is_open());
        stream.write(reinterpret_cast<const char*>(raw.data()), static_cast<std::streamsize>(raw.size() * sizeof(raw[0])));
    }

    outdoor::sensors::Icm20608CharReader reader(path.string());
    outdoor::sensors::Icm20608Sample sample;
    std::string error;
    assert(reader.read(sample, error));
    assert(almostEqual(sample.gyroXDps, 10.0));
    assert(almostEqual(sample.gyroYDps, -5.0));
    assert(almostEqual(sample.gyroZDps, 0.0));
    assert(almostEqual(sample.accelXG, 1.0));
    assert(almostEqual(sample.accelYG, -0.5));
    assert(almostEqual(sample.accelZG, 0.0));
    assert(almostEqual(sample.temperatureCelsius, 35.0));

    std::filesystem::remove(path);
    assert(!reader.read(sample, error));
    assert(error.find("failed to open") != std::string::npos);
    return 0;
}
