#include "services/icm20608_service.h"

#include "log/logger.h"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>
#include <utility>

namespace outdoor::services {

Icm20608Service::Icm20608Service(std::string source,
                                 outdoor::sensors::Icm20608CharReader charReader,
                                 outdoor::sensors::Icm20608IioReader reader,
                                 outdoor::sensors::BoardImuStatus& status,
                                 std::size_t sampleCount,
                                 std::uint32_t sampleIntervalMs)
    : requestedSource_(std::move(source)),
      charReader_(std::move(charReader)),
      reader_(std::move(reader)),
      status_(status),
      sampleCount_(sampleCount == 0U ? 1U : sampleCount),
      sampleIntervalMs_(sampleIntervalMs) {}

const char* Icm20608Service::name() const
{
    return "icm20608_service";
}

bool Icm20608Service::start()
{
    status_.enabled = true;
    status_.lastError.clear();

    std::string error;
    if (!selectSource(error)) {
        status_.lastError = error;
        outdoor::log::Logger::error(status_.lastError);
        return false;
    }

    outdoor::log::Logger::info("ICM20608 input selected: " + status_.source + " at " + status_.devicePath);
    return true;
}

bool Icm20608Service::run()
{
    for (std::size_t index = 0; index < sampleCount_; ++index) {
        outdoor::sensors::Icm20608Sample sample;
        std::string error;
        if (!readSample(sample, error)) {
            status_.lastError = error;
            outdoor::log::Logger::error(error);
            return false;
        }

        status_.seen = true;
        status_.sampleCount = index + 1U;
        status_.accelXG = sample.accelXG;
        status_.accelYG = sample.accelYG;
        status_.accelZG = sample.accelZG;
        status_.gyroXDps = sample.gyroXDps;
        status_.gyroYDps = sample.gyroYDps;
        status_.gyroZDps = sample.gyroZDps;
        status_.temperatureCelsius = sample.temperatureCelsius;
        status_.lastError.clear();

        std::ostringstream message;
        message << "ICM20608 sample: accel_g=(" << sample.accelXG << ", " << sample.accelYG << ", "
                << sample.accelZG << "), gyro_dps=(" << sample.gyroXDps << ", " << sample.gyroYDps << ", "
                << sample.gyroZDps << "), temp_c=" << sample.temperatureCelsius;
        outdoor::log::Logger::info(message.str());

        if (sampleIntervalMs_ > 0U && index + 1U < sampleCount_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sampleIntervalMs_));
        }
    }

    return true;
}

void Icm20608Service::stop()
{
}

bool Icm20608Service::selectSource(std::string& error)
{
    const bool allowChar = requestedSource_ == "char_device" || requestedSource_ == "auto";
    const bool allowIio = requestedSource_ == "iio" || requestedSource_ == "auto";

    if (allowChar && std::filesystem::exists(charReader_.devicePath())) {
        activeSource_ = "char_device";
        status_.source = "icm20608_char";
        status_.devicePath = charReader_.devicePath();
        error.clear();
        return true;
    }

    if (allowIio && std::filesystem::is_directory(reader_.devicePath())) {
        activeSource_ = "iio";
        status_.source = "icm20608_iio";
        status_.devicePath = reader_.devicePath();
        error.clear();
        return true;
    }

    if (requestedSource_ == "char_device") {
        error = "ICM20608 character device path does not exist: " + charReader_.devicePath();
    } else if (requestedSource_ == "iio") {
        error = "ICM20608 IIO device path does not exist: " + reader_.devicePath();
    } else {
        error = "no ICM20608 input source found; checked character device " + charReader_.devicePath()
            + " and IIO path " + reader_.devicePath();
    }
    return false;
}

bool Icm20608Service::readSample(outdoor::sensors::Icm20608Sample& sample, std::string& error) const
{
    if (activeSource_ == "char_device") {
        return charReader_.read(sample, error);
    }
    if (activeSource_ == "iio") {
        return reader_.read(sample, error);
    }

    error = "ICM20608 input source was not selected";
    return false;
}

} // namespace outdoor::services
