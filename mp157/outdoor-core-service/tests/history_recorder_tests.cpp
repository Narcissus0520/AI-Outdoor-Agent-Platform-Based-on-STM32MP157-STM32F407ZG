#include "storage/history_recorder.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

std::size_t lineCount(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    std::size_t count = 0U;
    std::string line;
    while (std::getline(stream, line)) {
        ++count;
    }
    return count;
}

std::string readText(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    namespace fs = std::filesystem;

    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path output = fs::temp_directory_path()
        / ("outdoor_history_recorder_test_" + std::to_string(unique));

    outdoor::gnss::GnssStatus gnss;
    outdoor::mcu::McuStatus mcu;
    outdoor::sensors::BoardImuStatus boardImu;
    outdoor::storage::HistoryRecorder recorder(output.string(), 0U, gnss, mcu, boardImu);

    std::string error;
    expect(recorder.start(error), "history recorder should start: " + error);

    gnss.seen = true;
    gnss.validSentenceCount = 1U;
    gnss.lastSentenceType = "RMC";
    gnss.fix.valid = true;
    gnss.fix.utcTime = "024603.00";
    gnss.fix.latitudeDegrees = 22.5411453;
    gnss.fix.longitudeDegrees = 114.0728752;

    mcu.imuSeen = true;
    mcu.imuSample.uptimeMs = 10U;
    mcu.imuSample.accelZMg = 1000;
    mcu.magnetometerSeen = true;
    mcu.magnetometerSample.uptimeMs = 20U;
    mcu.magnetometerSample.magneticXNt = 12345;
    mcu.barometerSeen = true;
    mcu.barometerSample.uptimeMs = 30U;
    mcu.barometerSample.pressurePa = 101325U;

    boardImu.seen = true;
    boardImu.sampleCount = 1U;
    boardImu.source = "char,device";
    boardImu.accelZG = 1.0;

    expect(recorder.recordSnapshot(error), "first history snapshot should be recorded: " + error);
    expect(recorder.recordSnapshot(error), "duplicate history snapshot should be accepted: " + error);

    gnss.validSentenceCount = 2U;
    gnss.lastSentenceType = "GGA";
    mcu.imuSample.uptimeMs = 40U;
    mcu.magnetometerSample.uptimeMs = 50U;
    mcu.barometerSample.uptimeMs = 60U;
    boardImu.sampleCount = 2U;
    expect(recorder.recordSnapshot(error), "second history snapshot should be recorded: " + error);
    recorder.stop();

    expect(lineCount(output / "gnss.csv") == 3U, "GNSS history should contain a header and two unique records");
    expect(lineCount(output / "mcu_imu.csv") == 3U, "MCU IMU history should contain two unique records");
    expect(lineCount(output / "magnetometer.csv") == 3U, "magnetometer history should contain two unique records");
    expect(lineCount(output / "barometer.csv") == 3U, "barometer history should contain two unique records");
    expect(lineCount(output / "board_imu.csv") == 3U, "board IMU history should contain two unique records");
    expect(readText(output / "board_imu.csv").find("\"char,device\"") != std::string::npos,
           "CSV string fields should be escaped");

    fs::remove_all(output);
    std::cout << "history_recorder_tests passed\n";
    return 0;
}
