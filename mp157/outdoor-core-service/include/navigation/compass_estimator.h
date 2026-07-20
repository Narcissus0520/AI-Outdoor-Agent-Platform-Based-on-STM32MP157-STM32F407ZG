#pragma once

#include "protocol/imu_payload.h"
#include "protocol/magnetometer_payload.h"

#include <array>
#include <cstdint>
#include <string>

namespace outdoor::navigation {

struct CompassConfig {
    bool enabled = true;
    bool calibrationValid = false;
    double declinationDegrees = 0.0;
    std::array<double, 3> hardIronBiasMicroTesla {0.0, 0.0, 0.0};
    std::array<double, 9> softIronMatrix {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
    double minimumFieldMicroTesla = 5.0;
    double maximumFieldMicroTesla = 100.0;
    double minimumAccelerationG = 0.70;
    double maximumAccelerationG = 1.30;
    std::uint32_t maximumSampleAgeMs = 250U;
};

struct CompassStatus {
    bool enabled = false;
    bool valid = false;
    bool calibrationApplied = false;
    bool tiltCompensated = false;
    std::uint32_t imuUptimeMs = 0U;
    std::uint32_t magnetometerUptimeMs = 0U;
    double magneticHeadingDegrees = 0.0;
    double headingDegrees = 0.0;
    double rollDegrees = 0.0;
    double pitchDegrees = 0.0;
    double fieldStrengthMicroTesla = 0.0;
    std::string quality = "unavailable";
    std::string lastError;
};

bool validateCompassConfig(const CompassConfig& config, std::string& error);

class CompassEstimator {
public:
    explicit CompassEstimator(CompassConfig config);

    CompassStatus estimate(bool imuSeen,
                           const outdoor::protocol::ImuSample& imu,
                           bool magnetometerSeen,
                           const outdoor::protocol::MagnetometerSample& magnetometer,
                           bool sensorSourcesHealthy = true) const;

private:
    CompassConfig config_;
};

} // namespace outdoor::navigation
