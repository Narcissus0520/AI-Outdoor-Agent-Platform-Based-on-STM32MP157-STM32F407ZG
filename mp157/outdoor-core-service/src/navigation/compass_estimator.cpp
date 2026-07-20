#include "navigation/compass_estimator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace outdoor::navigation {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kRadiansToDegrees = 180.0 / kPi;
constexpr double kMinimumHorizontalFieldMicroTesla = 0.001;
constexpr double kMinimumMatrixDeterminant = 1.0e-9;

double normalizeDegrees(double value)
{
    value = std::fmod(value, 360.0);
    if (value < 0.0) {
        value += 360.0;
    }
    return value;
}

bool finiteArray(const std::array<double, 3>& values)
{
    return std::all_of(values.begin(), values.end(), [](double value) {
        return std::isfinite(value);
    });
}

bool finiteArray(const std::array<double, 9>& values)
{
    return std::all_of(values.begin(), values.end(), [](double value) {
        return std::isfinite(value);
    });
}

double determinant(const std::array<double, 9>& matrix)
{
    return matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7])
         - matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6])
         + matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]);
}

std::array<double, 3> applyCalibration(const CompassConfig& config,
                                       const std::array<double, 3>& raw)
{
    const std::array<double, 3> centered {
        raw[0] - config.hardIronBiasMicroTesla[0],
        raw[1] - config.hardIronBiasMicroTesla[1],
        raw[2] - config.hardIronBiasMicroTesla[2],
    };

    const auto& matrix = config.softIronMatrix;
    return {
        matrix[0] * centered[0] + matrix[1] * centered[1] + matrix[2] * centered[2],
        matrix[3] * centered[0] + matrix[4] * centered[1] + matrix[5] * centered[2],
        matrix[6] * centered[0] + matrix[7] * centered[1] + matrix[8] * centered[2],
    };
}

double vectorNorm(double x, double y, double z)
{
    return std::sqrt(x * x + y * y + z * z);
}

std::uint32_t sampleAgeMs(std::uint32_t first, std::uint32_t second)
{
    const std::int64_t signedDelta = static_cast<std::int32_t>(first - second);
    const std::int64_t magnitude = signedDelta < 0 ? -signedDelta : signedDelta;
    return static_cast<std::uint32_t>(magnitude);
}

} // namespace

bool validateCompassConfig(const CompassConfig& config, std::string& error)
{
    if (!std::isfinite(config.declinationDegrees)) {
        error = "compass declination must be finite";
        return false;
    }
    if (!finiteArray(config.hardIronBiasMicroTesla)) {
        error = "compass hard-iron bias must contain finite values";
        return false;
    }
    if (!finiteArray(config.softIronMatrix)) {
        error = "compass soft-iron matrix must contain finite values";
        return false;
    }
    if (std::fabs(determinant(config.softIronMatrix)) < kMinimumMatrixDeterminant) {
        error = "compass soft-iron matrix must be non-singular";
        return false;
    }
    if (!std::isfinite(config.minimumFieldMicroTesla)
        || !std::isfinite(config.maximumFieldMicroTesla)
        || config.minimumFieldMicroTesla <= 0.0
        || config.maximumFieldMicroTesla <= config.minimumFieldMicroTesla) {
        error = "compass field range is invalid";
        return false;
    }
    if (!std::isfinite(config.minimumAccelerationG)
        || !std::isfinite(config.maximumAccelerationG)
        || config.minimumAccelerationG <= 0.0
        || config.maximumAccelerationG <= config.minimumAccelerationG) {
        error = "compass acceleration range is invalid";
        return false;
    }
    if (config.maximumSampleAgeMs == 0U) {
        error = "compass maximum sample age must be greater than zero";
        return false;
    }

    error.clear();
    return true;
}

CompassEstimator::CompassEstimator(CompassConfig config)
    : config_(std::move(config))
{
}

CompassStatus CompassEstimator::estimate(
    bool imuSeen,
    const outdoor::protocol::ImuSample& imu,
    bool magnetometerSeen,
    const outdoor::protocol::MagnetometerSample& magnetometer,
    bool sensorSourcesHealthy) const
{
    CompassStatus status;
    status.enabled = config_.enabled;
    status.calibrationApplied = config_.calibrationValid;
    status.imuUptimeMs = imu.uptimeMs;
    status.magnetometerUptimeMs = magnetometer.uptimeMs;

    if (!config_.enabled) {
        status.quality = "disabled";
        return status;
    }
    if (!imuSeen || !magnetometerSeen) {
        status.lastError = "compass requires both IMU and magnetometer samples";
        return status;
    }
    if (!sensorSourcesHealthy) {
        status.lastError = "compass source health flags do not confirm real IMU and magnetometer data";
        return status;
    }
    if (sampleAgeMs(imu.uptimeMs, magnetometer.uptimeMs) > config_.maximumSampleAgeMs) {
        status.lastError = "compass IMU and magnetometer samples are too far apart";
        return status;
    }

    const double accelX = outdoor::protocol::accelMgToG(imu.accelXMg);
    const double accelY = outdoor::protocol::accelMgToG(imu.accelYMg);
    const double accelZ = outdoor::protocol::accelMgToG(imu.accelZMg);
    const double accelerationNorm = vectorNorm(accelX, accelY, accelZ);
    if (!std::isfinite(accelerationNorm)
        || accelerationNorm < config_.minimumAccelerationG
        || accelerationNorm > config_.maximumAccelerationG) {
        status.lastError = "compass acceleration magnitude is outside the configured static range";
        return status;
    }

    const std::array<double, 3> rawMagnetic {
        outdoor::protocol::nanoTeslaToMicroTesla(magnetometer.magneticXNt),
        outdoor::protocol::nanoTeslaToMicroTesla(magnetometer.magneticYNt),
        outdoor::protocol::nanoTeslaToMicroTesla(magnetometer.magneticZNt),
    };
    const auto magnetic = config_.calibrationValid
        ? applyCalibration(config_, rawMagnetic)
        : rawMagnetic;
    status.fieldStrengthMicroTesla = vectorNorm(magnetic[0], magnetic[1], magnetic[2]);
    if (!std::isfinite(status.fieldStrengthMicroTesla)
        || status.fieldStrengthMicroTesla < config_.minimumFieldMicroTesla
        || status.fieldStrengthMicroTesla > config_.maximumFieldMicroTesla) {
        status.lastError = "compass magnetic field magnitude is outside the configured range";
        return status;
    }

    const double roll = std::atan2(accelY, accelZ);
    const double pitch = std::atan2(-accelX, std::sqrt(accelY * accelY + accelZ * accelZ));
    const double sinRoll = std::sin(roll);
    const double cosRoll = std::cos(roll);
    const double sinPitch = std::sin(pitch);
    const double cosPitch = std::cos(pitch);
    const double horizontalX = magnetic[0] * cosPitch + magnetic[2] * sinPitch;
    const double horizontalY = magnetic[0] * sinRoll * sinPitch
                             + magnetic[1] * cosRoll
                             - magnetic[2] * sinRoll * cosPitch;
    if (vectorNorm(horizontalX, horizontalY, 0.0) < kMinimumHorizontalFieldMicroTesla) {
        status.lastError = "compass horizontal magnetic field is too small";
        return status;
    }

    status.rollDegrees = roll * kRadiansToDegrees;
    status.pitchDegrees = pitch * kRadiansToDegrees;
    status.magneticHeadingDegrees = normalizeDegrees(
        std::atan2(-horizontalY, horizontalX) * kRadiansToDegrees);
    status.headingDegrees = normalizeDegrees(
        status.magneticHeadingDegrees + config_.declinationDegrees);
    status.tiltCompensated = true;
    status.valid = true;
    status.quality = config_.calibrationValid ? "calibrated" : "uncalibrated";
    status.lastError.clear();
    return status;
}

} // namespace outdoor::navigation
