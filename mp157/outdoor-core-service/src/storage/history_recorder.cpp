#include "storage/history_recorder.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>

namespace outdoor::storage {

namespace {

std::string currentUtcTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm utcTime {};
#if defined(_WIN32)
    gmtime_s(&utcTime, &time);
#else
    gmtime_r(&time, &utcTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setw(3) << std::setfill('0') << milliseconds << 'Z';
    return stream.str();
}

std::string escapeCsv(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }

    std::string escaped;
    escaped.reserve(value.size() + 2U);
    escaped.push_back('"');
    for (const char ch : value) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

bool openCsv(const std::filesystem::path& path,
             const char* header,
             std::ofstream& stream,
             std::string& error)
{
    namespace fs = std::filesystem;

    bool needsHeader = true;
    try {
        needsHeader = !fs::exists(path) || fs::file_size(path) == 0U;
        stream.open(path, std::ios::app);
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        return false;
    }

    if (!stream.is_open()) {
        error = "failed to open history file: " + path.string();
        return false;
    }
    if (needsHeader) {
        stream << header << '\n';
        if (!stream) {
            error = "failed to write history header: " + path.string();
            return false;
        }
    }

    return true;
}

bool streamHealthy(const std::ofstream& stream)
{
    return static_cast<bool>(stream);
}

} // namespace

HistoryRecorder::HistoryRecorder(std::string outputDirectory,
                                 std::uint32_t flushIntervalMs,
                                 const outdoor::gnss::GnssStatus& gnssStatus,
                                 const outdoor::mcu::McuStatus& mcuStatus,
                                 const outdoor::sensors::BoardImuStatus& boardImuStatus)
    : outputDirectory_(std::move(outputDirectory)),
      flushIntervalMs_(flushIntervalMs),
      gnssStatus_(gnssStatus),
      mcuStatus_(mcuStatus),
      boardImuStatus_(boardImuStatus)
{
}

HistoryRecorder::~HistoryRecorder()
{
    stop();
}

bool HistoryRecorder::start(std::string& error)
{
    namespace fs = std::filesystem;

    stop();
    if (outputDirectory_.empty()) {
        error = "history output directory must not be empty";
        return false;
    }

    try {
        fs::create_directories(outputDirectory_);
    } catch (const fs::filesystem_error& exception) {
        error = exception.what();
        return false;
    }

    const fs::path root(outputDirectory_);
    if (!openCsv(root / "gnss.csv",
                 "host_time_utc,valid_sentence_count,sentence_type,fix_valid,gnss_utc,latitude_degrees,longitude_degrees,altitude_meters,speed_kmh,course_degrees,satellite_count,satellites_in_view,fix_quality,fix_type,hdop,pdop,vdop",
                 gnssStream_, error)
        || !openCsv(root / "mcu_imu.csv",
                    "host_time_utc,uptime_ms,accel_x_mg,accel_y_mg,accel_z_mg,gyro_x_mdps,gyro_y_mdps,gyro_z_mdps,temperature_centi_c",
                    mcuImuStream_, error)
        || !openCsv(root / "magnetometer.csv",
                    "host_time_utc,uptime_ms,magnetic_x_nt,magnetic_y_nt,magnetic_z_nt",
                    magnetometerStream_, error)
        || !openCsv(root / "barometer.csv",
                    "host_time_utc,uptime_ms,pressure_pa,temperature_centi_c",
                    barometerStream_, error)
        || !openCsv(root / "board_imu.csv",
                    "host_time_utc,sample_count,source,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,gyro_z_dps,temperature_celsius",
                    boardImuStream_, error)) {
        stop();
        return false;
    }

    lastGnssSentenceCount_ = 0U;
    lastMcuImuUptimeMs_ = 0U;
    lastMagnetometerUptimeMs_ = 0U;
    lastBarometerUptimeMs_ = 0U;
    lastBoardImuSampleCount_ = 0U;
    mcuImuRecorded_ = false;
    magnetometerRecorded_ = false;
    barometerRecorded_ = false;
    nextFlushAt_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(flushIntervalMs_);
    started_ = true;
    error.clear();
    return true;
}

bool HistoryRecorder::recordSnapshot(std::string& error)
{
    if (!started_) {
        error = "history recorder is not started";
        return false;
    }

    const bool gnssChanged = gnssStatus_.seen
        && gnssStatus_.validSentenceCount != lastGnssSentenceCount_;
    const bool mcuImuChanged = mcuStatus_.imuSeen
        && (!mcuImuRecorded_ || mcuStatus_.imuSample.uptimeMs != lastMcuImuUptimeMs_);
    const bool magnetometerChanged = mcuStatus_.magnetometerSeen
        && (!magnetometerRecorded_
            || mcuStatus_.magnetometerSample.uptimeMs != lastMagnetometerUptimeMs_);
    const bool barometerChanged = mcuStatus_.barometerSeen
        && (!barometerRecorded_ || mcuStatus_.barometerSample.uptimeMs != lastBarometerUptimeMs_);
    const bool boardImuChanged = boardImuStatus_.seen
        && boardImuStatus_.sampleCount != lastBoardImuSampleCount_;

    if (!gnssChanged && !mcuImuChanged && !magnetometerChanged
        && !barometerChanged && !boardImuChanged) {
        return flushIfDue(error);
    }

    const std::string hostTime = currentUtcTimestamp();

    if (gnssChanged) {
        const auto& fix = gnssStatus_.fix;
        gnssStream_ << std::setprecision(10)
                    << hostTime << ','
                    << gnssStatus_.validSentenceCount << ','
                    << escapeCsv(gnssStatus_.lastSentenceType) << ','
                    << (fix.valid ? 1 : 0) << ','
                    << escapeCsv(fix.utcTime) << ','
                    << fix.latitudeDegrees << ','
                    << fix.longitudeDegrees << ','
                    << fix.altitudeMeters << ','
                    << fix.speedKmh << ','
                    << fix.courseDegrees << ','
                    << fix.satelliteCount << ','
                    << fix.satellitesInView << ','
                    << fix.fixQuality << ','
                    << fix.fixType << ','
                    << fix.hdop << ','
                    << fix.pdop << ','
                    << fix.vdop << '\n';
        lastGnssSentenceCount_ = gnssStatus_.validSentenceCount;
    }

    if (mcuImuChanged) {
        const auto& sample = mcuStatus_.imuSample;
        mcuImuStream_ << hostTime << ','
                      << sample.uptimeMs << ','
                      << sample.accelXMg << ','
                      << sample.accelYMg << ','
                      << sample.accelZMg << ','
                      << sample.gyroXMdps << ','
                      << sample.gyroYMdps << ','
                      << sample.gyroZMdps << ','
                      << sample.temperatureCentiC << '\n';
        lastMcuImuUptimeMs_ = sample.uptimeMs;
        mcuImuRecorded_ = true;
    }

    if (magnetometerChanged) {
        const auto& sample = mcuStatus_.magnetometerSample;
        magnetometerStream_ << hostTime << ','
                            << sample.uptimeMs << ','
                            << sample.magneticXNt << ','
                            << sample.magneticYNt << ','
                            << sample.magneticZNt << '\n';
        lastMagnetometerUptimeMs_ = sample.uptimeMs;
        magnetometerRecorded_ = true;
    }

    if (barometerChanged) {
        const auto& sample = mcuStatus_.barometerSample;
        barometerStream_ << hostTime << ','
                         << sample.uptimeMs << ','
                         << sample.pressurePa << ','
                         << sample.temperatureCentiC << '\n';
        lastBarometerUptimeMs_ = sample.uptimeMs;
        barometerRecorded_ = true;
    }

    if (boardImuChanged) {
        boardImuStream_ << std::setprecision(10)
                        << hostTime << ','
                        << boardImuStatus_.sampleCount << ','
                        << escapeCsv(boardImuStatus_.source) << ','
                        << boardImuStatus_.accelXG << ','
                        << boardImuStatus_.accelYG << ','
                        << boardImuStatus_.accelZG << ','
                        << boardImuStatus_.gyroXDps << ','
                        << boardImuStatus_.gyroYDps << ','
                        << boardImuStatus_.gyroZDps << ','
                        << boardImuStatus_.temperatureCelsius << '\n';
        lastBoardImuSampleCount_ = boardImuStatus_.sampleCount;
    }

    if (!streamHealthy(gnssStream_) || !streamHealthy(mcuImuStream_)
        || !streamHealthy(magnetometerStream_) || !streamHealthy(barometerStream_)
        || !streamHealthy(boardImuStream_)) {
        error = "failed to append a history record under: " + outputDirectory_;
        return false;
    }

    return flushIfDue(error);
}

void HistoryRecorder::stop()
{
    if (started_) {
        std::string ignoredError;
        (void)flushAll(ignoredError);
    }
    gnssStream_.close();
    mcuImuStream_.close();
    magnetometerStream_.close();
    barometerStream_.close();
    boardImuStream_.close();
    started_ = false;
}

const std::string& HistoryRecorder::outputDirectory() const
{
    return outputDirectory_;
}

bool HistoryRecorder::flushIfDue(std::string& error)
{
    const auto now = std::chrono::steady_clock::now();
    if (flushIntervalMs_ > 0U && now < nextFlushAt_) {
        error.clear();
        return true;
    }

    if (!flushAll(error)) {
        return false;
    }
    nextFlushAt_ = now + std::chrono::milliseconds(flushIntervalMs_);
    return true;
}

bool HistoryRecorder::flushAll(std::string& error)
{
    gnssStream_.flush();
    mcuImuStream_.flush();
    magnetometerStream_.flush();
    barometerStream_.flush();
    boardImuStream_.flush();

    if (!streamHealthy(gnssStream_) || !streamHealthy(mcuImuStream_)
        || !streamHealthy(magnetometerStream_) || !streamHealthy(barometerStream_)
        || !streamHealthy(boardImuStream_)) {
        error = "failed to flush history files under: " + outputDirectory_;
        return false;
    }

    error.clear();
    return true;
}

} // namespace outdoor::storage
