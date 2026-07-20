#include "calibration/compass_calibrator.h"

#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

struct CommandLine {
    std::string inputPath;
    std::string outputPath;
    outdoor::calibration::CompassCalibrationOptions options;
};

void printUsage(const char* executable)
{
    std::cout
        << "Usage: " << executable << " --input <magnetometer.csv> [options]\n"
        << "Options:\n"
        << "  --output <candidate.conf>          Write candidate Runtime config values\n"
        << "  --min-samples <count>             Default: 200\n"
        << "  --min-octants <1..8>              Default: 8\n"
        << "  --max-rms-percent <value>         Default: 5\n"
        << "  --max-residual-percent <value>    Default: 15\n"
        << "  --max-axis-ratio <value>          Default: 5\n"
        << "  --min-directional-ratio <value>   Default: 0.08\n"
        << "  --outlier-sigma <value>           Default: 4\n"
        << "  --sensor-to-body <9 CSV values>  Proper rotation; default: identity\n"
        << "  --no-outlier-rejection            Disable one-pass robust trimming\n"
        << "  --help                             Show this help\n";
}

bool parseUnsigned(const std::string& text, std::size_t& value)
{
    try {
        std::size_t consumed = 0U;
        const unsigned long long parsed = std::stoull(text, &consumed, 10);
        if (consumed != text.size()
            || parsed > static_cast<unsigned long long>(
                std::numeric_limits<std::size_t>::max())) {
            return false;
        }
        value = static_cast<std::size_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseDouble(const std::string& text, double& value)
{
    try {
        std::size_t consumed = 0U;
        value = std::stod(text, &consumed);
        return consumed == text.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

bool parseMatrix(const std::string& text,
                 std::array<double, 9>& matrix)
{
    std::size_t start = 0U;
    for (std::size_t index = 0; index < matrix.size(); ++index) {
        const std::size_t separator = text.find(',', start);
        if ((separator == std::string::npos) != (index + 1U == matrix.size())) {
            return false;
        }
        const std::string field = separator == std::string::npos
            ? text.substr(start)
            : text.substr(start, separator - start);
        if (!parseDouble(field, matrix[index])) {
            return false;
        }
        start = separator == std::string::npos ? text.size() : separator + 1U;
    }
    return start == text.size();
}

bool requireValue(int argc,
                  char* argv[],
                  int& index,
                  std::string& value,
                  std::string& error)
{
    if (index + 1 >= argc) {
        error = std::string("missing value after ") + argv[index];
        return false;
    }
    value = argv[++index];
    return true;
}

bool parseCommandLine(int argc,
                      char* argv[],
                      CommandLine& commandLine,
                      std::string& error)
{
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        }
        if (argument == "--no-outlier-rejection") {
            commandLine.options.rejectOutliers = false;
            continue;
        }

        std::string value;
        if (!requireValue(argc, argv, index, value, error)) {
            return false;
        }
        if (argument == "--input") {
            commandLine.inputPath = value;
        } else if (argument == "--output") {
            commandLine.outputPath = value;
        } else if (argument == "--min-samples") {
            if (!parseUnsigned(value, commandLine.options.minimumSampleCount)) {
                error = "invalid --min-samples value: " + value;
                return false;
            }
        } else if (argument == "--min-octants") {
            if (!parseUnsigned(value, commandLine.options.minimumOctants)) {
                error = "invalid --min-octants value: " + value;
                return false;
            }
        } else if (argument == "--max-rms-percent") {
            if (!parseDouble(value, commandLine.options.maximumRmsResidualPercent)) {
                error = "invalid --max-rms-percent value: " + value;
                return false;
            }
        } else if (argument == "--max-residual-percent") {
            if (!parseDouble(value, commandLine.options.maximumResidualPercent)) {
                error = "invalid --max-residual-percent value: " + value;
                return false;
            }
        } else if (argument == "--max-axis-ratio") {
            if (!parseDouble(value, commandLine.options.maximumAxisRatio)) {
                error = "invalid --max-axis-ratio value: " + value;
                return false;
            }
        } else if (argument == "--min-directional-ratio") {
            if (!parseDouble(value, commandLine.options.minimumDirectionalVarianceRatio)) {
                error = "invalid --min-directional-ratio value: " + value;
                return false;
            }
        } else if (argument == "--outlier-sigma") {
            if (!parseDouble(value, commandLine.options.outlierSigma)) {
                error = "invalid --outlier-sigma value: " + value;
                return false;
            }
        } else if (argument == "--sensor-to-body") {
            if (!parseMatrix(value, commandLine.options.sensorToBodyMatrix)) {
                error = "invalid --sensor-to-body matrix: " + value;
                return false;
            }
        } else {
            error = "unknown argument: " + argument;
            return false;
        }
    }
    if (commandLine.inputPath.empty()) {
        error = "--input is required";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    CommandLine commandLine;
    std::string error;
    if (!parseCommandLine(argc, argv, commandLine, error)) {
        std::cerr << "argument_error=" << error << '\n';
        printUsage(argv[0]);
        return 2;
    }

    std::vector<outdoor::calibration::MagneticSample> samples;
    if (!outdoor::calibration::loadMagnetometerHistoryCsv(
            commandLine.inputPath, samples, error)) {
        std::cerr << "input_error=" << error << '\n';
        return 2;
    }

    const auto result = outdoor::calibration::calibrateCompass(
        samples, commandLine.options);
    if (!result.fitted) {
        std::cerr << "fit_status=FAILED\nfit_error=" << result.error << '\n';
        return 1;
    }

    std::cout << std::fixed << std::setprecision(6)
              << "fit_status=" << (result.qualityPassed ? "PASSED" : "QUALITY_FAILED") << '\n'
              << "input_samples=" << result.inputSampleCount << '\n'
              << "used_samples=" << result.usedSampleCount << '\n'
              << "rejected_outliers=" << result.rejectedOutlierCount << '\n'
              << "sensor_to_body_applied=" << (result.sensorToBodyApplied ? "true" : "false") << '\n'
              << "octants_covered=" << result.octantsCovered << '\n'
              << "directional_variance_ratio=" << result.directionalVarianceRatio << '\n'
              << "axis_ratio=" << result.axisRatio << '\n'
              << "raw_field_min_ut=" << result.rawFieldMinimumMicroTesla << '\n'
              << "raw_field_max_ut=" << result.rawFieldMaximumMicroTesla << '\n'
              << "target_field_ut=" << result.targetFieldMicroTesla << '\n'
              << "corrected_field_mean_ut=" << result.correctedFieldMeanMicroTesla << '\n'
              << "corrected_field_stddev_ut=" << result.correctedFieldStandardDeviationMicroTesla << '\n'
              << "rms_residual_percent=" << result.rmsResidualPercent << '\n'
              << "max_residual_percent=" << result.maximumResidualPercent << '\n';
    for (const auto& failure : result.qualityFailures) {
        std::cout << "quality_failure=" << failure << '\n';
    }
    const std::string candidateConfig =
        outdoor::calibration::formatCompassCandidateConfig(result);
    std::cout << candidateConfig;

    if (!commandLine.outputPath.empty()) {
        std::ofstream output(commandLine.outputPath, std::ios::trunc);
        if (!output.is_open()) {
            std::cerr << "output_error=failed to open " << commandLine.outputPath << '\n';
            return 2;
        }
        output << candidateConfig;
        if (!output) {
            std::cerr << "output_error=failed to write " << commandLine.outputPath << '\n';
            return 2;
        }
    }

    return result.qualityPassed ? 0 : 1;
}
