#include "navigation/compass_estimator.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

bool nearlyEqual(double actual, double expected, double tolerance = 0.05)
{
    return std::fabs(actual - expected) <= tolerance;
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

outdoor::protocol::ImuSample levelImu(std::uint32_t uptimeMs = 1000U)
{
    outdoor::protocol::ImuSample sample;
    sample.uptimeMs = uptimeMs;
    sample.accelZMg = 1000;
    return sample;
}

outdoor::protocol::MagnetometerSample magneticSample(double xMicroTesla,
                                                      double yMicroTesla,
                                                      double zMicroTesla,
                                                      std::uint32_t uptimeMs = 1000U)
{
    outdoor::protocol::MagnetometerSample sample;
    sample.uptimeMs = uptimeMs;
    sample.magneticXNt = static_cast<std::int32_t>(std::lround(xMicroTesla * 1000.0));
    sample.magneticYNt = static_cast<std::int32_t>(std::lround(yMicroTesla * 1000.0));
    sample.magneticZNt = static_cast<std::int32_t>(std::lround(zMicroTesla * 1000.0));
    return sample;
}

} // namespace

int main()
{
    outdoor::navigation::CompassConfig config;
    std::string error;
    require(outdoor::navigation::validateCompassConfig(config, error),
            "default compass configuration is invalid: " + error);

    outdoor::navigation::CompassEstimator estimator(config);
    auto status = estimator.estimate(true, levelImu(), true, magneticSample(50.0, 0.0, 0.0));
    require(status.valid, "north estimate failed: " + status.lastError);
    require(nearlyEqual(status.headingDegrees, 0.0), "north heading mismatch");
    require(status.quality == "uncalibrated", "default estimate must be marked uncalibrated");

    status = estimator.estimate(true, levelImu(), true, magneticSample(0.0, -50.0, 0.0));
    require(status.valid, "east estimate failed: " + status.lastError);
    require(nearlyEqual(status.headingDegrees, 90.0), "east heading mismatch");

    config.declinationDegrees = 12.5;
    outdoor::navigation::CompassEstimator declinationEstimator(config);
    status = declinationEstimator.estimate(
        true, levelImu(), true, magneticSample(0.0, -50.0, 0.0));
    require(status.valid, "declination estimate failed: " + status.lastError);
    require(nearlyEqual(status.magneticHeadingDegrees, 90.0), "magnetic heading mismatch");
    require(nearlyEqual(status.headingDegrees, 102.5), "declination heading mismatch");

    config.declinationDegrees = 0.0;
    config.calibrationValid = true;
    config.hardIronBiasMicroTesla = {10.0, 5.0, -3.0};
    outdoor::navigation::CompassEstimator calibratedEstimator(config);
    status = calibratedEstimator.estimate(
        true, levelImu(), true, magneticSample(60.0, 5.0, -3.0));
    require(status.valid, "hard-iron estimate failed: " + status.lastError);
    require(status.calibrationApplied, "calibration was not reported as applied");
    require(status.quality == "calibrated", "calibrated quality mismatch");
    require(nearlyEqual(status.headingDegrees, 0.0), "hard-iron corrected heading mismatch");

    config = outdoor::navigation::CompassConfig {};
    outdoor::navigation::CompassEstimator tiltEstimator(config);
    auto tiltedImu = levelImu();
    tiltedImu.accelXMg = -500;
    tiltedImu.accelZMg = 866;
    status = tiltEstimator.estimate(
        true, tiltedImu, true, magneticSample(43.301, 0.0, 25.0));
    require(status.valid, "tilt-compensated estimate failed: " + status.lastError);
    require(status.tiltCompensated, "tilt compensation was not reported");
    require(nearlyEqual(status.pitchDegrees, 30.0, 0.1), "pitch estimate mismatch");
    require(nearlyEqual(status.headingDegrees, 0.0, 0.1), "tilt-compensated heading mismatch");

    status = tiltEstimator.estimate(false, tiltedImu, true, magneticSample(40.0, 0.0, 0.0));
    require(!status.valid, "missing IMU unexpectedly produced a heading");
    require(status.lastError.find("both IMU and magnetometer") != std::string::npos,
            "missing-input error mismatch");

    status = tiltEstimator.estimate(
        true, levelImu(1000U), true, magneticSample(40.0, 0.0, 0.0, 2000U));
    require(!status.valid, "stale sensor pair unexpectedly produced a heading");
    require(status.lastError.find("too far apart") != std::string::npos,
            "stale-sample error mismatch");

    status = tiltEstimator.estimate(true, levelImu(), true, magneticSample(1.0, 0.0, 0.0));
    require(!status.valid, "out-of-range field unexpectedly produced a heading");
    require(status.lastError.find("magnetic field magnitude") != std::string::npos,
            "field-range error mismatch");

    status = tiltEstimator.estimate(
        true, levelImu(), true, magneticSample(40.0, 0.0, 0.0), false);
    require(!status.valid, "unhealthy source flags unexpectedly produced a heading");
    require(status.lastError.find("source health flags") != std::string::npos,
            "source-health error mismatch");

    config.softIronMatrix = {
        1.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
    };
    require(!outdoor::navigation::validateCompassConfig(config, error),
            "singular soft-iron matrix was accepted");
    require(error.find("non-singular") != std::string::npos,
            "singular-matrix error mismatch");

    return 0;
}
