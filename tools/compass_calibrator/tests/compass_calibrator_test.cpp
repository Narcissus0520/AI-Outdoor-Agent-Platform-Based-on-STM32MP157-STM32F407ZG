#include "calibration/compass_calibrator.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

using outdoor::calibration::MagneticSample;
using Matrix3 = std::array<double, 9>;

void check(bool condition, const char* expression, const char* file, int line)
{
    if (!condition) {
        std::cerr << "CHECK failed: " << expression
                  << " at " << file << ':' << line << '\n';
        std::exit(1);
    }
}

#define CHECK(expression) check((expression), #expression, __FILE__, __LINE__)

double determinant(const Matrix3& matrix)
{
    return matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7])
         - matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6])
         + matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]);
}

Matrix3 inverse(const Matrix3& matrix)
{
    const double det = determinant(matrix);
    CHECK(std::fabs(det) > 1.0e-9);
    return {
        (matrix[4] * matrix[8] - matrix[5] * matrix[7]) / det,
        (matrix[2] * matrix[7] - matrix[1] * matrix[8]) / det,
        (matrix[1] * matrix[5] - matrix[2] * matrix[4]) / det,
        (matrix[5] * matrix[6] - matrix[3] * matrix[8]) / det,
        (matrix[0] * matrix[8] - matrix[2] * matrix[6]) / det,
        (matrix[2] * matrix[3] - matrix[0] * matrix[5]) / det,
        (matrix[3] * matrix[7] - matrix[4] * matrix[6]) / det,
        (matrix[1] * matrix[6] - matrix[0] * matrix[7]) / det,
        (matrix[0] * matrix[4] - matrix[1] * matrix[3]) / det,
    };
}

MagneticSample multiply(const Matrix3& matrix, const MagneticSample& sample)
{
    return {
        matrix[0] * sample.xMicroTesla + matrix[1] * sample.yMicroTesla + matrix[2] * sample.zMicroTesla,
        matrix[3] * sample.xMicroTesla + matrix[4] * sample.yMicroTesla + matrix[5] * sample.zMicroTesla,
        matrix[6] * sample.xMicroTesla + matrix[7] * sample.yMicroTesla + matrix[8] * sample.zMicroTesla,
    };
}

std::vector<MagneticSample> makeDistortedSphere(std::size_t count,
                                                const Matrix3& correction,
                                                const std::array<double, 3>& bias,
                                                double fieldMicroTesla)
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kGoldenAngle = kPi * (3.0 - 2.2360679774997896964);
    const Matrix3 distortion = inverse(correction);
    std::vector<MagneticSample> samples;
    samples.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const double z = 1.0 - 2.0 * (static_cast<double>(index) + 0.5)
                               / static_cast<double>(count);
        const double radius = std::sqrt(std::max(0.0, 1.0 - z * z));
        const double angle = kGoldenAngle * static_cast<double>(index);
        const MagneticSample field {
            fieldMicroTesla * radius * std::cos(angle),
            fieldMicroTesla * radius * std::sin(angle),
            fieldMicroTesla * z,
        };
        MagneticSample raw = multiply(distortion, field);
        const double deterministicNoise = 0.015 * std::sin(static_cast<double>(index) * 0.37);
        raw.xMicroTesla += bias[0] + deterministicNoise;
        raw.yMicroTesla += bias[1] - 0.7 * deterministicNoise;
        raw.zMicroTesla += bias[2] + 0.4 * deterministicNoise;
        samples.push_back(raw);
    }
    return samples;
}

void testRecoversFullMatrixAndBias()
{
    Matrix3 correction {
        1.18, 0.09, 0.03,
        0.09, 0.86, -0.05,
        0.03, -0.05, 1.02,
    };
    const double determinantScale = std::pow(determinant(correction), -1.0 / 3.0);
    for (double& value : correction) {
        value *= determinantScale;
    }
    const std::array<double, 3> bias {12.5, -8.25, 4.75};
    auto samples = makeDistortedSphere(1600U, correction, bias, 48.0);

    outdoor::calibration::CompassCalibrationOptions options;
    options.minimumSampleCount = 500U;
    const auto result = outdoor::calibration::calibrateCompass(samples, options);
    CHECK(result.fitted);
    CHECK(result.qualityPassed);
    CHECK(result.octantsCovered == 8U);
    CHECK(result.directionalVarianceRatio > 0.3);
    CHECK(result.rmsResidualPercent < 0.2);
    CHECK(result.maximumResidualPercent < 0.5);
    for (std::size_t index = 0; index < 3U; ++index) {
        CHECK(std::fabs(result.hardIronBiasMicroTesla[index] - bias[index]) < 0.05);
    }
    for (std::size_t index = 0; index < correction.size(); ++index) {
        CHECK(std::fabs(result.softIronMatrix[index] - correction[index]) < 0.01);
    }
    const std::string candidate =
        outdoor::calibration::formatCompassCandidateConfig(result);
    CHECK(candidate.find("compass_calibration_valid = false") != std::string::npos);
    CHECK(candidate.find("compass_hard_iron_x_ut = ") != std::string::npos);
    CHECK(candidate.find("compass_soft_iron_22 = ") != std::string::npos);
}

void testRejectsPlanarCoverage()
{
    std::vector<MagneticSample> samples;
    for (std::size_t index = 0; index < 500U; ++index) {
        const double angle = static_cast<double>(index) * 0.05;
        samples.push_back({40.0 * std::cos(angle), 30.0 * std::sin(angle), 2.0});
    }
    const auto result = outdoor::calibration::calibrateCompass(samples);
    CHECK(!result.fitted || !result.qualityPassed);
}

void testRejectsGrossOutliers()
{
    Matrix3 correction {
        1.12, 0.06, 0.02,
        0.06, 0.91, -0.03,
        0.02, -0.03, 0.99,
    };
    const double determinantScale = std::pow(determinant(correction), -1.0 / 3.0);
    for (double& value : correction) {
        value *= determinantScale;
    }
    const std::array<double, 3> bias {7.0, -5.0, 3.0};
    auto samples = makeDistortedSphere(1600U, correction, bias, 50.0);
    samples.push_back({250.0, 250.0, 250.0});
    samples.push_back({-250.0, 250.0, 250.0});
    samples.push_back({250.0, -250.0, 250.0});
    samples.push_back({250.0, 250.0, -250.0});
    samples.push_back({-250.0, -250.0, -250.0});

    outdoor::calibration::CompassCalibrationOptions options;
    options.minimumSampleCount = 500U;
    const auto result = outdoor::calibration::calibrateCompass(samples, options);
    CHECK(result.fitted);
    CHECK(result.qualityPassed);
    CHECK(result.rejectedOutlierCount >= 5U);
    CHECK(result.rmsResidualPercent < 0.5);
}

void testComposesSensorToBodyRotation()
{
    Matrix3 correction {
        1.10, 0.05, 0.01,
        0.05, 0.92, -0.02,
        0.01, -0.02, 0.99,
    };
    const double determinantScale = std::pow(determinant(correction), -1.0 / 3.0);
    for (double& value : correction) {
        value *= determinantScale;
    }
    auto samples = makeDistortedSphere(1200U, correction, {2.0, -3.0, 5.0}, 46.0);
    outdoor::calibration::CompassCalibrationOptions options;
    options.minimumSampleCount = 500U;
    const Matrix3 sensorToBody {
        0.0, -1.0, 0.0,
        1.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
    };
    options.sensorToBodyMatrix = sensorToBody;
    const auto result = outdoor::calibration::calibrateCompass(samples, options);
    CHECK(result.fitted);
    CHECK(result.qualityPassed);
    CHECK(result.sensorToBodyApplied);
    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t column = 0; column < 3U; ++column) {
            double expected = 0.0;
            for (std::size_t inner = 0; inner < 3U; ++inner) {
                expected += sensorToBody[row * 3U + inner]
                          * correction[inner * 3U + column];
            }
            CHECK(std::fabs(result.softIronMatrix[row * 3U + column] - expected) < 0.01);
        }
    }
}

void testRejectsInvalidSensorToBodyMatrix()
{
    auto samples = makeDistortedSphere(
        500U,
        {
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
        },
        {0.0, 0.0, 0.0},
        50.0);
    outdoor::calibration::CompassCalibrationOptions options;
    options.sensorToBodyMatrix[0] = 2.0;
    const auto result = outdoor::calibration::calibrateCompass(samples, options);
    CHECK(!result.fitted);
    CHECK(result.error == "compass calibration options are invalid");
}

void testHistoryCsvLoader()
{
    const auto path = std::filesystem::temp_directory_path()
        / "outdoor_compass_calibrator_history.csv";
    {
        std::ofstream stream(path, std::ios::trunc);
        stream << "host_time_utc,uptime_ms,magnetic_x_nt,magnetic_y_nt,magnetic_z_nt\n"
               << "2026-07-19T00:00:00.000Z,10,12500,-8250,4750\n"
               << "\"2026-07-19T00:00:00.010Z\",20,13000,-8000,5000\n";
    }
    std::vector<MagneticSample> samples;
    std::string error;
    CHECK(outdoor::calibration::loadMagnetometerHistoryCsv(
        path.string(), samples, error));
    CHECK(samples.size() == 2U);
    CHECK(std::fabs(samples[0].xMicroTesla - 12.5) < 1.0e-9);
    CHECK(std::fabs(samples[0].yMicroTesla + 8.25) < 1.0e-9);
    CHECK(std::fabs(samples[1].zMicroTesla - 5.0) < 1.0e-9);
    std::filesystem::remove(path);
}

} // namespace

int main()
{
    testRecoversFullMatrixAndBias();
    testRejectsPlanarCoverage();
    testRejectsGrossOutliers();
    testComposesSensorToBodyRotation();
    testRejectsInvalidSensorToBodyMatrix();
    testHistoryCsvLoader();
    return 0;
}
