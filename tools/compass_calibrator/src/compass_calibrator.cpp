#include "calibration/compass_calibrator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace outdoor::calibration {

namespace {

constexpr std::size_t kFitParameterCount = 9U;
constexpr double kNumericalTolerance = 1.0e-12;

using Matrix3 = std::array<double, 9>;
using Vector3 = std::array<double, 3>;
using NormalMatrix = std::array<std::array<double, kFitParameterCount + 1U>,
                                kFitParameterCount>;

struct EllipsoidFit {
    bool valid = false;
    std::string error;
    Vector3 center {0.0, 0.0, 0.0};
    Matrix3 shape {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
    Matrix3 correction {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
    double targetFieldMicroTesla = 0.0;
    double axisRatio = 0.0;
};

double square(double value)
{
    return value * value;
}

double norm(const MagneticSample& sample)
{
    return std::sqrt(square(sample.xMicroTesla)
                     + square(sample.yMicroTesla)
                     + square(sample.zMicroTesla));
}

bool finiteSample(const MagneticSample& sample)
{
    return std::isfinite(sample.xMicroTesla)
        && std::isfinite(sample.yMicroTesla)
        && std::isfinite(sample.zMicroTesla);
}

double determinant(const Matrix3& matrix)
{
    return matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7])
         - matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6])
         + matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]);
}

bool invert(const Matrix3& matrix, Matrix3& inverse)
{
    const double det = determinant(matrix);
    double largest = 0.0;
    for (const double value : matrix) {
        largest = std::max(largest, std::fabs(value));
    }
    if (!std::isfinite(det)
        || largest <= kNumericalTolerance
        || std::fabs(det) <= kNumericalTolerance * largest * largest * largest) {
        return false;
    }

    inverse = {
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
    return true;
}

Vector3 multiply(const Matrix3& matrix, const Vector3& vector)
{
    return {
        matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2],
        matrix[3] * vector[0] + matrix[4] * vector[1] + matrix[5] * vector[2],
        matrix[6] * vector[0] + matrix[7] * vector[1] + matrix[8] * vector[2],
    };
}

double dot(const Vector3& first, const Vector3& second)
{
    return first[0] * second[0] + first[1] * second[1] + first[2] * second[2];
}

bool solveNormalEquations(NormalMatrix augmented,
                          std::array<double, kFitParameterCount>& solution)
{
    double largestCoefficient = 0.0;
    for (const auto& row : augmented) {
        for (std::size_t column = 0; column < kFitParameterCount; ++column) {
            largestCoefficient = std::max(largestCoefficient, std::fabs(row[column]));
        }
    }
    if (largestCoefficient <= kNumericalTolerance) {
        return false;
    }

    for (std::size_t pivotColumn = 0; pivotColumn < kFitParameterCount; ++pivotColumn) {
        std::size_t pivotRow = pivotColumn;
        for (std::size_t row = pivotColumn + 1U; row < kFitParameterCount; ++row) {
            if (std::fabs(augmented[row][pivotColumn])
                > std::fabs(augmented[pivotRow][pivotColumn])) {
                pivotRow = row;
            }
        }
        if (std::fabs(augmented[pivotRow][pivotColumn])
            <= kNumericalTolerance * largestCoefficient) {
            return false;
        }
        if (pivotRow != pivotColumn) {
            std::swap(augmented[pivotRow], augmented[pivotColumn]);
        }

        const double pivot = augmented[pivotColumn][pivotColumn];
        for (std::size_t column = pivotColumn;
             column <= kFitParameterCount;
             ++column) {
            augmented[pivotColumn][column] /= pivot;
        }

        for (std::size_t row = 0; row < kFitParameterCount; ++row) {
            if (row == pivotColumn) {
                continue;
            }
            const double factor = augmented[row][pivotColumn];
            for (std::size_t column = pivotColumn;
                 column <= kFitParameterCount;
                 ++column) {
                augmented[row][column] -= factor * augmented[pivotColumn][column];
            }
        }
    }

    for (std::size_t index = 0; index < kFitParameterCount; ++index) {
        solution[index] = augmented[index][kFitParameterCount];
        if (!std::isfinite(solution[index])) {
            return false;
        }
    }
    return true;
}

void jacobiEigenDecomposition(const Matrix3& input,
                              Vector3& eigenvalues,
                              Matrix3& eigenvectors)
{
    Matrix3 matrix = input;
    eigenvectors = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };

    for (int iteration = 0; iteration < 64; ++iteration) {
        std::size_t p = 0U;
        std::size_t q = 1U;
        double largest = std::fabs(matrix[1]);
        if (std::fabs(matrix[2]) > largest) {
            p = 0U;
            q = 2U;
            largest = std::fabs(matrix[2]);
        }
        if (std::fabs(matrix[5]) > largest) {
            p = 1U;
            q = 2U;
            largest = std::fabs(matrix[5]);
        }
        if (largest <= kNumericalTolerance) {
            break;
        }

        const double app = matrix[p * 3U + p];
        const double aqq = matrix[q * 3U + q];
        const double apq = matrix[p * 3U + q];
        const double angle = 0.5 * std::atan2(2.0 * apq, aqq - app);
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);

        for (std::size_t index = 0; index < 3U; ++index) {
            if (index == p || index == q) {
                continue;
            }
            const double aip = matrix[index * 3U + p];
            const double aiq = matrix[index * 3U + q];
            const double rotatedP = cosine * aip - sine * aiq;
            const double rotatedQ = sine * aip + cosine * aiq;
            matrix[index * 3U + p] = rotatedP;
            matrix[p * 3U + index] = rotatedP;
            matrix[index * 3U + q] = rotatedQ;
            matrix[q * 3U + index] = rotatedQ;
        }

        matrix[p * 3U + p] = cosine * cosine * app
                            - 2.0 * sine * cosine * apq
                            + sine * sine * aqq;
        matrix[q * 3U + q] = sine * sine * app
                            + 2.0 * sine * cosine * apq
                            + cosine * cosine * aqq;
        matrix[p * 3U + q] = 0.0;
        matrix[q * 3U + p] = 0.0;

        for (std::size_t row = 0; row < 3U; ++row) {
            const double vip = eigenvectors[row * 3U + p];
            const double viq = eigenvectors[row * 3U + q];
            eigenvectors[row * 3U + p] = cosine * vip - sine * viq;
            eigenvectors[row * 3U + q] = sine * vip + cosine * viq;
        }
    }

    eigenvalues = {matrix[0], matrix[4], matrix[8]};
}

Matrix3 symmetricFromEigen(const Matrix3& eigenvectors,
                           const Vector3& diagonal)
{
    Matrix3 result {};
    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t column = 0; column < 3U; ++column) {
            double value = 0.0;
            for (std::size_t eigen = 0; eigen < 3U; ++eigen) {
                value += eigenvectors[row * 3U + eigen]
                       * diagonal[eigen]
                       * eigenvectors[column * 3U + eigen];
            }
            result[row * 3U + column] = value;
        }
    }
    return result;
}

Matrix3 multiplyMatrices(const Matrix3& first, const Matrix3& second)
{
    Matrix3 result {};
    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t column = 0; column < 3U; ++column) {
            for (std::size_t inner = 0; inner < 3U; ++inner) {
                result[row * 3U + column] += first[row * 3U + inner]
                                            * second[inner * 3U + column];
            }
        }
    }
    return result;
}

bool properRotationMatrix(const Matrix3& matrix)
{
    for (const double value : matrix) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    constexpr double kRotationTolerance = 1.0e-4;
    for (std::size_t first = 0; first < 3U; ++first) {
        for (std::size_t second = 0; second < 3U; ++second) {
            double product = 0.0;
            for (std::size_t column = 0; column < 3U; ++column) {
                product += matrix[first * 3U + column]
                         * matrix[second * 3U + column];
            }
            const double expected = first == second ? 1.0 : 0.0;
            if (std::fabs(product - expected) > kRotationTolerance) {
                return false;
            }
        }
    }
    return std::fabs(determinant(matrix) - 1.0) <= kRotationTolerance;
}

bool identityMatrix(const Matrix3& matrix)
{
    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t column = 0; column < 3U; ++column) {
            const double expected = row == column ? 1.0 : 0.0;
            if (std::fabs(matrix[row * 3U + column] - expected) > 1.0e-9) {
                return false;
            }
        }
    }
    return true;
}

EllipsoidFit fitEllipsoid(const std::vector<MagneticSample>& samples,
                          const std::vector<std::size_t>& indices)
{
    EllipsoidFit fit;
    if (indices.size() < kFitParameterCount + 1U) {
        fit.error = "not enough samples for a 3D ellipsoid fit";
        return fit;
    }

    Vector3 mean {0.0, 0.0, 0.0};
    for (const std::size_t index : indices) {
        mean[0] += samples[index].xMicroTesla;
        mean[1] += samples[index].yMicroTesla;
        mean[2] += samples[index].zMicroTesla;
    }
    for (double& value : mean) {
        value /= static_cast<double>(indices.size());
    }

    double scaleSquared = 0.0;
    for (const std::size_t index : indices) {
        scaleSquared += square(samples[index].xMicroTesla - mean[0])
                      + square(samples[index].yMicroTesla - mean[1])
                      + square(samples[index].zMicroTesla - mean[2]);
    }
    const double normalizationScale = std::sqrt(
        scaleSquared / static_cast<double>(indices.size()));
    if (!std::isfinite(normalizationScale)
        || normalizationScale <= kNumericalTolerance) {
        fit.error = "magnetometer samples do not span a finite 3D range";
        return fit;
    }

    NormalMatrix normal {};
    for (const std::size_t index : indices) {
        const double x = (samples[index].xMicroTesla - mean[0]) / normalizationScale;
        const double y = (samples[index].yMicroTesla - mean[1]) / normalizationScale;
        const double z = (samples[index].zMicroTesla - mean[2]) / normalizationScale;
        const std::array<double, kFitParameterCount> row {
            x * x, y * y, z * z,
            2.0 * x * y, 2.0 * x * z, 2.0 * y * z,
            x, y, z,
        };
        for (std::size_t first = 0; first < kFitParameterCount; ++first) {
            for (std::size_t second = 0; second < kFitParameterCount; ++second) {
                normal[first][second] += row[first] * row[second];
            }
            normal[first][kFitParameterCount] += row[first];
        }
    }

    std::array<double, kFitParameterCount> parameters {};
    if (!solveNormalEquations(normal, parameters)) {
        fit.error = "ellipsoid least-squares system is singular; collect more 3D orientations";
        return fit;
    }

    const Matrix3 quadratic {
        parameters[0], parameters[3], parameters[4],
        parameters[3], parameters[1], parameters[5],
        parameters[4], parameters[5], parameters[2],
    };
    Matrix3 inverseQuadratic {};
    if (!invert(quadratic, inverseQuadratic)) {
        fit.error = "fitted ellipsoid quadratic matrix is singular";
        return fit;
    }
    const Vector3 linear {parameters[6], parameters[7], parameters[8]};
    Vector3 centerNormalized = multiply(inverseQuadratic, linear);
    for (double& value : centerNormalized) {
        value *= -0.5;
    }

    const Vector3 quadraticCenter = multiply(quadratic, centerNormalized);
    const double translatedLevel = 1.0 + dot(centerNormalized, quadraticCenter);
    if (!std::isfinite(translatedLevel)
        || translatedLevel <= kNumericalTolerance) {
        fit.error = "fitted ellipsoid has a non-positive translated level";
        return fit;
    }

    Matrix3 shape {};
    const double shapeScale = translatedLevel
                            * normalizationScale
                            * normalizationScale;
    for (std::size_t index = 0; index < shape.size(); ++index) {
        shape[index] = quadratic[index] / shapeScale;
    }

    Vector3 eigenvalues {};
    Matrix3 eigenvectors {};
    jacobiEigenDecomposition(shape, eigenvalues, eigenvectors);
    const double minimumEigenvalue = *std::min_element(eigenvalues.begin(), eigenvalues.end());
    const double maximumEigenvalue = *std::max_element(eigenvalues.begin(), eigenvalues.end());
    if (!std::isfinite(minimumEigenvalue)
        || !std::isfinite(maximumEigenvalue)
        || minimumEigenvalue <= kNumericalTolerance) {
        fit.error = "fitted ellipsoid is not positive definite; improve full-orientation coverage";
        return fit;
    }

    const double shapeDeterminant = eigenvalues[0] * eigenvalues[1] * eigenvalues[2];
    const double targetField = std::pow(shapeDeterminant, -1.0 / 6.0);
    if (!std::isfinite(targetField) || targetField <= 0.0) {
        fit.error = "fitted ellipsoid scale is invalid";
        return fit;
    }
    const Vector3 correctionEigenvalues {
        targetField * std::sqrt(eigenvalues[0]),
        targetField * std::sqrt(eigenvalues[1]),
        targetField * std::sqrt(eigenvalues[2]),
    };

    fit.center = {
        mean[0] + normalizationScale * centerNormalized[0],
        mean[1] + normalizationScale * centerNormalized[1],
        mean[2] + normalizationScale * centerNormalized[2],
    };
    fit.shape = shape;
    fit.correction = symmetricFromEigen(eigenvectors, correctionEigenvalues);
    fit.targetFieldMicroTesla = targetField;
    fit.axisRatio = std::sqrt(maximumEigenvalue / minimumEigenvalue);
    fit.valid = true;
    return fit;
}

double median(std::vector<double> values)
{
    if (values.empty()) {
        return 0.0;
    }
    const std::size_t middle = values.size() / 2U;
    std::nth_element(values.begin(), values.begin() + middle, values.end());
    const double upper = values[middle];
    if ((values.size() % 2U) != 0U) {
        return upper;
    }
    std::nth_element(values.begin(), values.begin() + middle - 1U, values.end());
    return 0.5 * (upper + values[middle - 1U]);
}

std::vector<double> relativeResiduals(const std::vector<MagneticSample>& samples,
                                      const std::vector<std::size_t>& indices,
                                      const EllipsoidFit& fit)
{
    std::vector<double> residuals;
    residuals.reserve(indices.size());
    for (const std::size_t index : indices) {
        const MagneticSample corrected = applyCompassCalibration(
            samples[index], fit.center, fit.correction);
        residuals.push_back(std::fabs(norm(corrected) / fit.targetFieldMicroTesla - 1.0));
    }
    return residuals;
}

std::size_t octantCoverage(const std::vector<MagneticSample>& samples,
                           const std::vector<std::size_t>& indices,
                           const Vector3& center)
{
    std::array<bool, 8> occupied {};
    for (const std::size_t index : indices) {
        std::size_t octant = 0U;
        if (samples[index].xMicroTesla >= center[0]) octant |= 1U;
        if (samples[index].yMicroTesla >= center[1]) octant |= 2U;
        if (samples[index].zMicroTesla >= center[2]) octant |= 4U;
        occupied[octant] = true;
    }
    return static_cast<std::size_t>(std::count(occupied.begin(), occupied.end(), true));
}

double directionalVarianceRatio(const std::vector<MagneticSample>& samples,
                                const std::vector<std::size_t>& indices,
                                const Vector3& center)
{
    Matrix3 covariance {};
    std::size_t count = 0U;
    for (const std::size_t index : indices) {
        Vector3 direction {
            samples[index].xMicroTesla - center[0],
            samples[index].yMicroTesla - center[1],
            samples[index].zMicroTesla - center[2],
        };
        const double length = std::sqrt(dot(direction, direction));
        if (length <= kNumericalTolerance) {
            continue;
        }
        for (double& value : direction) {
            value /= length;
        }
        for (std::size_t row = 0; row < 3U; ++row) {
            for (std::size_t column = 0; column < 3U; ++column) {
                covariance[row * 3U + column] += direction[row] * direction[column];
            }
        }
        ++count;
    }
    if (count == 0U) {
        return 0.0;
    }
    for (double& value : covariance) {
        value /= static_cast<double>(count);
    }
    Vector3 eigenvalues {};
    Matrix3 eigenvectors {};
    jacobiEigenDecomposition(covariance, eigenvalues, eigenvectors);
    const double minimum = *std::min_element(eigenvalues.begin(), eigenvalues.end());
    const double maximum = *std::max_element(eigenvalues.begin(), eigenvalues.end());
    if (maximum <= kNumericalTolerance) {
        return 0.0;
    }
    return std::max(0.0, minimum / maximum);
}

bool parseCsvLine(const std::string& line,
                  std::vector<std::string>& fields,
                  std::string& error)
{
    fields.clear();
    std::string field;
    bool quoted = false;
    for (std::size_t index = 0; index < line.size(); ++index) {
        const char ch = line[index];
        if (quoted) {
            if (ch == '"') {
                if (index + 1U < line.size() && line[index + 1U] == '"') {
                    field.push_back('"');
                    ++index;
                } else {
                    quoted = false;
                }
            } else {
                field.push_back(ch);
            }
        } else if (ch == ',') {
            fields.push_back(field);
            field.clear();
        } else if (ch == '"' && field.empty()) {
            quoted = true;
        } else {
            field.push_back(ch);
        }
    }
    if (quoted) {
        error = "unterminated quoted CSV field";
        return false;
    }
    fields.push_back(field);
    return true;
}

bool parseFiniteDouble(const std::string& text, double& value)
{
    try {
        std::size_t consumed = 0U;
        value = std::stod(text, &consumed);
        if (consumed != text.size() || !std::isfinite(value)) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::string formatNumber(double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << value;
    return stream.str();
}

} // namespace

bool loadMagnetometerHistoryCsv(const std::string& path,
                                std::vector<MagneticSample>& samples,
                                std::string& error)
{
    std::ifstream stream(path);
    if (!stream.is_open()) {
        error = "failed to open magnetometer CSV: " + path;
        return false;
    }

    std::string line;
    if (!std::getline(stream, line)) {
        error = "magnetometer CSV is empty: " + path;
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    std::vector<std::string> fields;
    if (!parseCsvLine(line, fields, error)) {
        error = "invalid CSV header: " + error;
        return false;
    }
    std::array<std::size_t, 3> columns {
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max(),
    };
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (fields[index] == "magnetic_x_nt") columns[0] = index;
        if (fields[index] == "magnetic_y_nt") columns[1] = index;
        if (fields[index] == "magnetic_z_nt") columns[2] = index;
    }
    if (std::any_of(columns.begin(), columns.end(), [](std::size_t column) {
            return column == std::numeric_limits<std::size_t>::max();
        })) {
        error = "CSV header must contain magnetic_x_nt, magnetic_y_nt, and magnetic_z_nt";
        return false;
    }

    samples.clear();
    std::size_t lineNumber = 1U;
    while (std::getline(stream, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (!parseCsvLine(line, fields, error)) {
            error = "invalid CSV at line " + std::to_string(lineNumber) + ": " + error;
            return false;
        }
        const std::size_t largestColumn = *std::max_element(columns.begin(), columns.end());
        if (fields.size() <= largestColumn) {
            error = "CSV row has too few fields at line " + std::to_string(lineNumber);
            return false;
        }

        MagneticSample sample;
        double xNanoTesla = 0.0;
        double yNanoTesla = 0.0;
        double zNanoTesla = 0.0;
        if (!parseFiniteDouble(fields[columns[0]], xNanoTesla)
            || !parseFiniteDouble(fields[columns[1]], yNanoTesla)
            || !parseFiniteDouble(fields[columns[2]], zNanoTesla)) {
            error = "CSV row contains an invalid magnetometer value at line "
                  + std::to_string(lineNumber);
            return false;
        }
        sample.xMicroTesla = xNanoTesla / 1000.0;
        sample.yMicroTesla = yNanoTesla / 1000.0;
        sample.zMicroTesla = zNanoTesla / 1000.0;
        samples.push_back(sample);
    }
    if (!stream.eof() && stream.fail()) {
        error = "failed while reading magnetometer CSV: " + path;
        return false;
    }
    if (samples.empty()) {
        error = "magnetometer CSV contains no data rows";
        return false;
    }

    error.clear();
    return true;
}

CompassCalibrationResult calibrateCompass(
    const std::vector<MagneticSample>& samples,
    const CompassCalibrationOptions& options)
{
    CompassCalibrationResult result;
    result.inputSampleCount = samples.size();
    if (options.minimumSampleCount < kFitParameterCount + 1U
        || options.minimumOctants == 0U
        || options.minimumOctants > 8U
        || !std::isfinite(options.maximumRmsResidualPercent)
        || options.maximumRmsResidualPercent <= 0.0
        || !std::isfinite(options.maximumResidualPercent)
        || options.maximumResidualPercent <= options.maximumRmsResidualPercent
        || !std::isfinite(options.maximumAxisRatio)
        || options.maximumAxisRatio <= 1.0
        || !std::isfinite(options.minimumDirectionalVarianceRatio)
        || options.minimumDirectionalVarianceRatio <= 0.0
        || options.minimumDirectionalVarianceRatio > 1.0
        || !std::isfinite(options.outlierSigma)
        || options.outlierSigma <= 0.0
        || !properRotationMatrix(options.sensorToBodyMatrix)) {
        result.error = "compass calibration options are invalid";
        return result;
    }
    if (samples.size() < options.minimumSampleCount) {
        result.error = "not enough magnetometer samples: got "
                     + std::to_string(samples.size())
                     + ", need at least "
                     + std::to_string(options.minimumSampleCount);
        return result;
    }
    for (std::size_t index = 0; index < samples.size(); ++index) {
        if (!finiteSample(samples[index])) {
            result.error = "magnetometer sample " + std::to_string(index)
                         + " contains a non-finite value";
            return result;
        }
    }

    result.rawFieldMinimumMicroTesla = std::numeric_limits<double>::infinity();
    result.rawFieldMaximumMicroTesla = 0.0;
    for (const auto& sample : samples) {
        const double field = norm(sample);
        result.rawFieldMinimumMicroTesla = std::min(result.rawFieldMinimumMicroTesla, field);
        result.rawFieldMaximumMicroTesla = std::max(result.rawFieldMaximumMicroTesla, field);
    }

    std::vector<std::size_t> indices(samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index) {
        indices[index] = index;
    }

    if (options.rejectOutliers) {
        std::vector<double> xValues;
        std::vector<double> yValues;
        std::vector<double> zValues;
        xValues.reserve(samples.size());
        yValues.reserve(samples.size());
        zValues.reserve(samples.size());
        for (const auto& sample : samples) {
            xValues.push_back(sample.xMicroTesla);
            yValues.push_back(sample.yMicroTesla);
            zValues.push_back(sample.zMicroTesla);
        }
        const Vector3 robustCenter {
            median(std::move(xValues)),
            median(std::move(yValues)),
            median(std::move(zValues)),
        };
        std::vector<double> distances;
        distances.reserve(samples.size());
        for (const auto& sample : samples) {
            const double x = sample.xMicroTesla - robustCenter[0];
            const double y = sample.yMicroTesla - robustCenter[1];
            const double z = sample.zMicroTesla - robustCenter[2];
            distances.push_back(std::sqrt(x * x + y * y + z * z));
        }
        const double medianDistance = median(distances);
        std::vector<double> distanceDeviations;
        distanceDeviations.reserve(distances.size());
        for (const double distance : distances) {
            distanceDeviations.push_back(std::fabs(distance - medianDistance));
        }
        const double distanceSigma = 1.4826 * median(std::move(distanceDeviations));
        const double grossThreshold = std::max(
            medianDistance * 3.0,
            medianDistance + options.outlierSigma * distanceSigma);
        std::vector<std::size_t> retained;
        retained.reserve(indices.size());
        for (std::size_t index = 0; index < distances.size(); ++index) {
            if (distances[index] <= grossThreshold) {
                retained.push_back(index);
            }
        }
        if (retained.size() >= options.minimumSampleCount
            && retained.size() < indices.size()) {
            result.rejectedOutlierCount = indices.size() - retained.size();
            indices = std::move(retained);
        }
    }

    EllipsoidFit fit = fitEllipsoid(samples, indices);
    if (!fit.valid) {
        result.error = fit.error;
        return result;
    }

    if (options.rejectOutliers) {
        const std::vector<double> residuals = relativeResiduals(samples, indices, fit);
        const double centerResidual = median(residuals);
        std::vector<double> deviations;
        deviations.reserve(residuals.size());
        for (const double residual : residuals) {
            deviations.push_back(std::fabs(residual - centerResidual));
        }
        const double robustSigma = 1.4826 * median(std::move(deviations));
        const double threshold = std::max(
            0.01, centerResidual + options.outlierSigma * robustSigma);

        std::vector<std::size_t> retained;
        retained.reserve(indices.size());
        for (std::size_t index = 0; index < indices.size(); ++index) {
            if (residuals[index] <= threshold) {
                retained.push_back(indices[index]);
            }
        }
        if (retained.size() >= options.minimumSampleCount
            && retained.size() < indices.size()) {
            const EllipsoidFit refined = fitEllipsoid(samples, retained);
            if (!refined.valid) {
                result.error = "outlier-refined fit failed: " + refined.error;
                return result;
            }
            result.rejectedOutlierCount += indices.size() - retained.size();
            indices = std::move(retained);
            fit = refined;
        }
    }

    result.fitted = true;
    result.usedSampleCount = indices.size();
    result.hardIronBiasMicroTesla = fit.center;
    result.sensorToBodyApplied = !identityMatrix(options.sensorToBodyMatrix);
    result.softIronMatrix = multiplyMatrices(options.sensorToBodyMatrix, fit.correction);
    result.targetFieldMicroTesla = fit.targetFieldMicroTesla;
    result.axisRatio = fit.axisRatio;
    result.octantsCovered = octantCoverage(samples, indices, fit.center);
    result.directionalVarianceRatio = directionalVarianceRatio(samples, indices, fit.center);

    double correctedSum = 0.0;
    double residualSquareSum = 0.0;
    double maximumResidual = 0.0;
    std::vector<double> correctedFields;
    correctedFields.reserve(indices.size());
    for (const std::size_t index : indices) {
        const MagneticSample corrected = applyCompassCalibration(
            samples[index], fit.center, fit.correction);
        const double field = norm(corrected);
        correctedFields.push_back(field);
        correctedSum += field;
        const double residual = std::fabs(field / fit.targetFieldMicroTesla - 1.0);
        residualSquareSum += residual * residual;
        maximumResidual = std::max(maximumResidual, residual);
    }
    result.correctedFieldMeanMicroTesla = correctedSum
        / static_cast<double>(correctedFields.size());
    double varianceSum = 0.0;
    for (const double field : correctedFields) {
        varianceSum += square(field - result.correctedFieldMeanMicroTesla);
    }
    result.correctedFieldStandardDeviationMicroTesla = std::sqrt(
        varianceSum / static_cast<double>(correctedFields.size()));
    result.rmsResidualPercent = 100.0 * std::sqrt(
        residualSquareSum / static_cast<double>(correctedFields.size()));
    result.maximumResidualPercent = 100.0 * maximumResidual;

    if (result.octantsCovered < options.minimumOctants) {
        result.qualityFailures.push_back(
            "orientation coverage is " + std::to_string(result.octantsCovered)
            + "/8 octants; need at least " + std::to_string(options.minimumOctants));
    }
    if (result.directionalVarianceRatio < options.minimumDirectionalVarianceRatio) {
        result.qualityFailures.push_back(
            "directional variance ratio " + formatNumber(result.directionalVarianceRatio)
            + " is below " + formatNumber(options.minimumDirectionalVarianceRatio));
    }
    if (result.axisRatio > options.maximumAxisRatio) {
        result.qualityFailures.push_back(
            "fitted ellipsoid axis ratio " + formatNumber(result.axisRatio)
            + " exceeds " + formatNumber(options.maximumAxisRatio));
    }
    if (result.rmsResidualPercent > options.maximumRmsResidualPercent) {
        result.qualityFailures.push_back(
            "RMS residual " + formatNumber(result.rmsResidualPercent)
            + "% exceeds " + formatNumber(options.maximumRmsResidualPercent) + "%");
    }
    if (result.maximumResidualPercent > options.maximumResidualPercent) {
        result.qualityFailures.push_back(
            "maximum residual " + formatNumber(result.maximumResidualPercent)
            + "% exceeds " + formatNumber(options.maximumResidualPercent) + "%");
    }
    result.qualityPassed = result.qualityFailures.empty();
    return result;
}

MagneticSample applyCompassCalibration(
    const MagneticSample& sample,
    const std::array<double, 3>& hardIronBiasMicroTesla,
    const std::array<double, 9>& softIronMatrix)
{
    const Vector3 centered {
        sample.xMicroTesla - hardIronBiasMicroTesla[0],
        sample.yMicroTesla - hardIronBiasMicroTesla[1],
        sample.zMicroTesla - hardIronBiasMicroTesla[2],
    };
    const Vector3 corrected = multiply(softIronMatrix, centered);
    return {corrected[0], corrected[1], corrected[2]};
}

std::string formatCompassCandidateConfig(const CompassCalibrationResult& result)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(9)
           << "# Candidate values generated from magnetometer.csv.\n"
           << "# The 3x3 matrix includes the supplied sensor-to-body rotation.\n"
           << "# Keep false until independent eight-direction and tilt validation passes.\n"
           << "compass_calibration_valid = false\n"
           << "compass_hard_iron_x_ut = " << result.hardIronBiasMicroTesla[0] << '\n'
           << "compass_hard_iron_y_ut = " << result.hardIronBiasMicroTesla[1] << '\n'
           << "compass_hard_iron_z_ut = " << result.hardIronBiasMicroTesla[2] << '\n';
    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t column = 0; column < 3U; ++column) {
            stream << "compass_soft_iron_" << row << column << " = "
                   << result.softIronMatrix[row * 3U + column] << '\n';
        }
    }
    return stream.str();
}

} // namespace outdoor::calibration
