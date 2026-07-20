#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace outdoor::calibration {

struct MagneticSample {
    double xMicroTesla = 0.0;
    double yMicroTesla = 0.0;
    double zMicroTesla = 0.0;
};

struct CompassCalibrationOptions {
    std::size_t minimumSampleCount = 200U;
    std::size_t minimumOctants = 8U;
    double maximumRmsResidualPercent = 5.0;
    double maximumResidualPercent = 15.0;
    double maximumAxisRatio = 5.0;
    double minimumDirectionalVarianceRatio = 0.08;
    bool rejectOutliers = true;
    double outlierSigma = 4.0;
    std::array<double, 9> sensorToBodyMatrix {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
};

struct CompassCalibrationResult {
    bool fitted = false;
    bool qualityPassed = false;
    bool sensorToBodyApplied = false;
    std::string error;
    std::vector<std::string> qualityFailures;
    std::size_t inputSampleCount = 0U;
    std::size_t usedSampleCount = 0U;
    std::size_t rejectedOutlierCount = 0U;
    std::size_t octantsCovered = 0U;
    std::array<double, 3> hardIronBiasMicroTesla {0.0, 0.0, 0.0};
    std::array<double, 9> softIronMatrix {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
    double targetFieldMicroTesla = 0.0;
    double rawFieldMinimumMicroTesla = 0.0;
    double rawFieldMaximumMicroTesla = 0.0;
    double correctedFieldMeanMicroTesla = 0.0;
    double correctedFieldStandardDeviationMicroTesla = 0.0;
    double rmsResidualPercent = 0.0;
    double maximumResidualPercent = 0.0;
    double axisRatio = 0.0;
    double directionalVarianceRatio = 0.0;
};

bool loadMagnetometerHistoryCsv(const std::string& path,
                                std::vector<MagneticSample>& samples,
                                std::string& error);

CompassCalibrationResult calibrateCompass(
    const std::vector<MagneticSample>& samples,
    const CompassCalibrationOptions& options = {});

MagneticSample applyCompassCalibration(
    const MagneticSample& sample,
    const std::array<double, 3>& hardIronBiasMicroTesla,
    const std::array<double, 9>& softIronMatrix);

std::string formatCompassCandidateConfig(const CompassCalibrationResult& result);

} // namespace outdoor::calibration
