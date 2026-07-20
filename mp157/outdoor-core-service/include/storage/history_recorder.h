#pragma once

#include "gnss/gnss_status.h"
#include "mcu/mcu_status.h"
#include "sensors/board_imu_status.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>

namespace outdoor::storage {

class HistoryRecorder {
public:
    HistoryRecorder(std::string outputDirectory,
                    std::uint32_t flushIntervalMs,
                    const outdoor::gnss::GnssStatus& gnssStatus,
                    const outdoor::mcu::McuStatus& mcuStatus,
                    const outdoor::sensors::BoardImuStatus& boardImuStatus);
    ~HistoryRecorder();

    bool start(std::string& error);
    bool recordSnapshot(std::string& error);
    void stop();

    const std::string& outputDirectory() const;

private:
    bool flushIfDue(std::string& error);
    bool flushAll(std::string& error);

    std::string outputDirectory_;
    std::uint32_t flushIntervalMs_ = 1000U;
    const outdoor::gnss::GnssStatus& gnssStatus_;
    const outdoor::mcu::McuStatus& mcuStatus_;
    const outdoor::sensors::BoardImuStatus& boardImuStatus_;
    std::ofstream gnssStream_;
    std::ofstream mcuImuStream_;
    std::ofstream magnetometerStream_;
    std::ofstream barometerStream_;
    std::ofstream boardImuStream_;
    std::chrono::steady_clock::time_point nextFlushAt_ {};
    std::uint32_t lastGnssSentenceCount_ = 0;
    std::uint32_t lastMcuImuUptimeMs_ = 0;
    std::uint32_t lastMagnetometerUptimeMs_ = 0;
    std::uint32_t lastBarometerUptimeMs_ = 0;
    std::size_t lastBoardImuSampleCount_ = 0;
    bool mcuImuRecorded_ = false;
    bool magnetometerRecorded_ = false;
    bool barometerRecorded_ = false;
    bool started_ = false;
};

} // namespace outdoor::storage
