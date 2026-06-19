#include "sensors/icm20608_iio_reader.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

void writeValue(const std::filesystem::path& directory, const std::string& name, const std::string& value)
{
    std::ofstream stream(directory / name, std::ios::trunc);
    assert(stream.is_open());
    stream << value << "\n";
}

bool almostEqual(double left, double right)
{
    return std::fabs(left - right) < 0.0005;
}

} // namespace

int main()
{
    const auto directory = std::filesystem::temp_directory_path() / "icm20608_iio_reader_tests";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    writeValue(directory, "in_accel_scale", "0.000488281");
    writeValue(directory, "in_accel_x_raw", "2048");
    writeValue(directory, "in_accel_y_raw", "-1024");
    writeValue(directory, "in_accel_z_raw", "0");
    writeValue(directory, "in_anglvel_scale", "0.061035");
    writeValue(directory, "in_anglvel_x_raw", "100");
    writeValue(directory, "in_anglvel_y_raw", "-50");
    writeValue(directory, "in_anglvel_z_raw", "0");
    writeValue(directory, "in_temp_scale", "326.8");
    writeValue(directory, "in_temp_offset", "0");
    writeValue(directory, "in_temp_raw", "3268");

    outdoor::sensors::Icm20608IioReader reader(directory.string());
    outdoor::sensors::Icm20608Sample sample;
    std::string error;
    assert(reader.read(sample, error));
    assert(almostEqual(sample.accelXG, 1.0));
    assert(almostEqual(sample.accelYG, -0.5));
    assert(almostEqual(sample.accelZG, 0.0));
    assert(almostEqual(sample.gyroXDps, 6.1035));
    assert(almostEqual(sample.gyroYDps, -3.05175));
    assert(almostEqual(sample.gyroZDps, 0.0));
    assert(almostEqual(sample.temperatureCelsius, 35.0));

    std::filesystem::remove(directory / "in_temp_scale");
    assert(!reader.read(sample, error));
    assert(error.find("in_temp_scale") != std::string::npos);

    std::filesystem::remove_all(directory);
    return 0;
}
