#include "services/dashboard_service.h"

#include "log/logger.h"
#include "protocol/imu_payload.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#endif

namespace outdoor::services {
namespace {

#ifndef _WIN32
constexpr std::uint8_t kGlyphWidth = 5;
constexpr std::uint8_t kGlyphHeight = 7;

struct Rgb {
    std::uint8_t r {};
    std::uint8_t g {};
    std::uint8_t b {};
};

std::string toUpperAscii(std::string_view value)
{
    std::string output;
    output.reserve(value.size());
    for (const char ch : value) {
        output.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    return output;
}

std::array<std::uint8_t, kGlyphHeight> glyphFor(char ch)
{
    switch (ch) {
    case 'A': return {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    case 'B': return {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
    case 'C': return {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
    case 'D': return {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
    case 'E': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    case 'F': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
    case 'G': return {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F};
    case 'H': return {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    case 'I': return {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
    case 'J': return {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
    case 'K': return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    case 'L': return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
    case 'M': return {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
    case 'N': return {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    case 'O': return {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    case 'P': return {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    case 'Q': return {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
    case 'R': return {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
    case 'S': return {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    case 'T': return {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    case 'U': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    case 'V': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
    case 'W': return {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
    case 'X': return {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
    case 'Y': return {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
    case 'Z': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
    case '0': return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
    case '1': return {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
    case '2': return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
    case '3': return {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
    case '4': return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
    case '5': return {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
    case '6': return {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
    case '7': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    case '8': return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
    case '9': return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
    case ':': return {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
    case '.': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
    case ',': return {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08};
    case '-': return {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
    case '+': return {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
    case '/': return {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10};
    case '_': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
    case '=': return {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00};
    case '%': return {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13};
    case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    default: return {0x1F, 0x01, 0x02, 0x04, 0x04, 0x00, 0x04};
    }
}

std::string boolText(bool value)
{
    return value ? "YES" : "NO";
}

std::string fixedValue(double value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string intValue(std::uint32_t value)
{
    return std::to_string(value);
}

std::uint32_t scaleToBitfield(std::uint8_t value, std::uint32_t length)
{
    if (length == 0) {
        return 0;
    }
    const std::uint32_t maxValue = (1u << length) - 1u;
    return (static_cast<std::uint32_t>(value) * maxValue + 127u) / 255u;
}

std::uint32_t packColor(const fb_var_screeninfo& info, Rgb color)
{
    return (scaleToBitfield(color.r, info.red.length) << info.red.offset) |
           (scaleToBitfield(color.g, info.green.length) << info.green.offset) |
           (scaleToBitfield(color.b, info.blue.length) << info.blue.offset);
}

class FramebufferCanvas {
public:
    explicit FramebufferCanvas(std::string device)
        : device_(std::move(device))
    {
    }

    FramebufferCanvas(const FramebufferCanvas&) = delete;
    FramebufferCanvas& operator=(const FramebufferCanvas&) = delete;

    ~FramebufferCanvas()
    {
        if (buffer_ != MAP_FAILED && buffer_ != nullptr) {
            munmap(buffer_, bufferLength_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    bool open()
    {
        fd_ = ::open(device_.c_str(), O_RDWR);
        if (fd_ < 0) {
            lastError_ = "open failed: " + std::string(std::strerror(errno));
            return false;
        }
        if (ioctl(fd_, FBIOGET_FSCREENINFO, &fixedInfo_) < 0) {
            lastError_ = "FBIOGET_FSCREENINFO failed: " + std::string(std::strerror(errno));
            return false;
        }
        if (ioctl(fd_, FBIOGET_VSCREENINFO, &varInfo_) < 0) {
            lastError_ = "FBIOGET_VSCREENINFO failed: " + std::string(std::strerror(errno));
            return false;
        }
        if (varInfo_.bits_per_pixel != 16 && varInfo_.bits_per_pixel != 24 && varInfo_.bits_per_pixel != 32) {
            lastError_ = "unsupported framebuffer bpp: " + std::to_string(varInfo_.bits_per_pixel);
            return false;
        }

        bufferLength_ = fixedInfo_.smem_len;
        buffer_ = static_cast<std::uint8_t*>(mmap(nullptr, bufferLength_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
        if (buffer_ == MAP_FAILED) {
            lastError_ = "mmap failed: " + std::string(std::strerror(errno));
            return false;
        }
        return true;
    }

    std::string lastError() const
    {
        return lastError_;
    }

    int width() const
    {
        return static_cast<int>(varInfo_.xres);
    }

    int height() const
    {
        return static_cast<int>(varInfo_.yres);
    }

    void fill(Rgb color)
    {
        fillRect(0, 0, width(), height(), color);
    }

    void fillRect(int x, int y, int width, int height, Rgb color)
    {
        const int x0 = std::max(0, x);
        const int y0 = std::max(0, y);
        const int x1 = std::min(this->width(), x + width);
        const int y1 = std::min(this->height(), y + height);
        for (int row = y0; row < y1; ++row) {
            for (int col = x0; col < x1; ++col) {
                setPixel(col, row, color);
            }
        }
    }

    void drawText(int x, int y, std::string_view text, Rgb color, int scale)
    {
        const std::string upperText = toUpperAscii(text);
        int cursorX = x;
        for (const char ch : upperText) {
            drawGlyph(cursorX, y, ch, color, scale);
            cursorX += (kGlyphWidth + 1) * scale;
        }
    }

private:
    void setPixel(int x, int y, Rgb color)
    {
        if (x < 0 || y < 0 || x >= width() || y >= height()) {
            return;
        }
        const int bytesPerPixel = static_cast<int>(varInfo_.bits_per_pixel / 8);
        const long location = (static_cast<long>(x) + static_cast<long>(varInfo_.xoffset)) * bytesPerPixel +
                              (static_cast<long>(y) + static_cast<long>(varInfo_.yoffset)) * fixedInfo_.line_length;
        if (location < 0 || static_cast<std::size_t>(location + bytesPerPixel) > bufferLength_) {
            return;
        }

        const std::uint32_t packed = packColor(varInfo_, color);
        if (varInfo_.bits_per_pixel == 16) {
            const std::uint16_t value = static_cast<std::uint16_t>(packed);
            std::memcpy(buffer_ + location, &value, sizeof(value));
        } else if (varInfo_.bits_per_pixel == 24) {
            buffer_[location + 0] = static_cast<std::uint8_t>((packed >> 0) & 0xFFu);
            buffer_[location + 1] = static_cast<std::uint8_t>((packed >> 8) & 0xFFu);
            buffer_[location + 2] = static_cast<std::uint8_t>((packed >> 16) & 0xFFu);
        } else {
            std::memcpy(buffer_ + location, &packed, sizeof(packed));
        }
    }

    void drawGlyph(int x, int y, char ch, Rgb color, int scale)
    {
        const auto glyph = glyphFor(ch);
        for (int row = 0; row < kGlyphHeight; ++row) {
            for (int col = 0; col < kGlyphWidth; ++col) {
                if ((glyph[static_cast<std::size_t>(row)] & (1u << (kGlyphWidth - 1 - col))) == 0) {
                    continue;
                }
                fillRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }

    std::string device_;
    int fd_ {-1};
    fb_fix_screeninfo fixedInfo_ {};
    fb_var_screeninfo varInfo_ {};
    std::uint8_t* buffer_ {nullptr};
    std::size_t bufferLength_ {};
    std::string lastError_;
};
#endif

} // namespace

DashboardService::DashboardService(std::string outputPath,
                                   std::string outputMode,
                                   std::string framebufferDevice,
                                   std::size_t refreshCount,
                                   std::uint32_t refreshIntervalMs,
                                   const outdoor::gnss::GnssStatus& gnssStatus,
                                   const outdoor::mcu::McuStatus& mcuStatus,
                                   const outdoor::sensors::BoardImuStatus& boardImuStatus)
    : outputPath_(std::move(outputPath)),
      outputMode_(std::move(outputMode)),
      framebufferDevice_(std::move(framebufferDevice)),
      refreshCount_(refreshCount),
      refreshIntervalMs_(refreshIntervalMs),
      gnssStatus_(gnssStatus),
      mcuStatus_(mcuStatus),
      boardImuStatus_(boardImuStatus)
{
}

const char* DashboardService::name() const
{
    return "dashboard_service";
}

bool DashboardService::start()
{
    return true;
}

bool DashboardService::run()
{
    if (outputMode_ != "text" && outputMode_ != "framebuffer" && outputMode_ != "both") {
        outdoor::log::Logger::error("Unsupported dashboard output mode: " + outputMode_);
        return false;
    }

    const std::size_t iterations = refreshCount_ == 0 ? 1 : refreshCount_;
    for (std::size_t index = 0; index < iterations; ++index) {
        bool ok = true;
        if (outputMode_ == "text" || outputMode_ == "both") {
            ok = writeTextDashboard() && ok;
        }
        if (outputMode_ == "framebuffer" || outputMode_ == "both") {
            ok = writeFramebufferDashboard() && ok;
        }
        if (!ok) {
            return false;
        }

        if (index + 1 < iterations && refreshIntervalMs_ > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(refreshIntervalMs_));
        }
    }
    return true;
}

void DashboardService::stop() {}

bool DashboardService::writeTextDashboard()
{
    namespace fs = std::filesystem;

    try {
        const fs::path path(outputPath_);
        const fs::path parent = path.parent_path();
        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        std::ofstream stream(path, std::ios::trunc);
        if (!stream.is_open()) {
            outdoor::log::Logger::error("Failed to open dashboard output: " + outputPath_);
            return false;
        }
        stream << render();
    } catch (const fs::filesystem_error& exception) {
        outdoor::log::Logger::error(std::string("Failed to write dashboard: ") + exception.what());
        return false;
    }

    outdoor::log::Logger::info("Dashboard rendered: " + outputPath_);
    return true;
}

bool DashboardService::writeFramebufferDashboard()
{
#ifdef _WIN32
    (void)framebufferDevice_;
    outdoor::log::Logger::error("Framebuffer dashboard is only available on Linux fbdev targets");
    return false;
#else
    FramebufferCanvas canvas(framebufferDevice_);
    if (!canvas.open()) {
        outdoor::log::Logger::error("Failed to open dashboard framebuffer " + framebufferDevice_ + ": " + canvas.lastError());
        return false;
    }

    const auto& fix = gnssStatus_.fix;
    const auto& imu = mcuStatus_.imuSample;
    const Rgb bg {6, 14, 30};
    const Rgb panel {16, 32, 58};
    const Rgb panelAlt {20, 44, 70};
    const Rgb cyan {98, 216, 255};
    const Rgb green {84, 222, 142};
    const Rgb yellow {255, 218, 94};
    const Rgb white {232, 238, 246};
    const Rgb muted {148, 165, 186};

    canvas.fill(bg);
    canvas.fillRect(0, 0, canvas.width(), 82, {10, 26, 51});
    canvas.drawText(32, 22, "OUTDOOR-AGENT", cyan, 4);
    canvas.drawText(34, 62, "FIELD DASHBOARD / AI AGENT TERMINAL", muted, 2);

    const int margin = 28;
    const int gap = 24;
    const int leftWidth = (canvas.width() - margin * 2 - gap) / 2;
    const int rightWidth = leftWidth;
    const int leftX = margin;
    const int rightX = margin + leftWidth + gap;
    const int topY = 110;
    const int midY = 342;
    const int bottomY = 468;

    auto drawPanel = [&](int x, int y, int width, int height, std::string_view title, Rgb accent) {
        canvas.fillRect(x, y, width, height, panel);
        canvas.fillRect(x, y, width, 6, accent);
        canvas.fillRect(x + 8, y + 42, width - 16, 2, panelAlt);
        canvas.drawText(x + 18, y + 18, title, accent, 2);
    };
    auto drawLine = [&](int x, int y, std::string_view label, std::string_view value, Rgb valueColor) {
        canvas.drawText(x, y, label, muted, 2);
        canvas.drawText(x + 182, y, value, valueColor, 2);
    };

    drawPanel(leftX, topY, leftWidth, 456, "GNSS / UBLOX M10", green);
    drawLine(leftX + 22, topY + 64, "FIX", boolText(fix.valid), fix.valid ? green : yellow);
    drawLine(leftX + 22, topY + 92, "LAT", fixedValue(fix.latitudeDegrees, 6), white);
    drawLine(leftX + 22, topY + 120, "LON", fixedValue(fix.longitudeDegrees, 6), white);
    drawLine(leftX + 22, topY + 148, "ALT M", fixedValue(fix.altitudeMeters, 1), white);
    drawLine(leftX + 22, topY + 176, "SPD KMH", fixedValue(fix.speedKmh, 1), white);
    drawLine(leftX + 22, topY + 204, "COURSE", fixedValue(fix.courseDegrees, 1), white);
    drawLine(leftX + 22, topY + 232, "SAT USED", intValue(static_cast<std::uint32_t>(fix.satelliteCount)), white);
    drawLine(leftX + 22, topY + 260, "SAT VIEW", intValue(static_cast<std::uint32_t>(fix.satellitesInView)), white);
    drawLine(leftX + 22, topY + 288, "HDOP", fixedValue(fix.hdop, 1), white);
    drawLine(leftX + 22, topY + 316, "UTC", fix.utcTime.empty() ? "N/A" : fix.utcTime, white);
    drawLine(leftX + 22, topY + 344, "SOURCE", gnssStatus_.source, white);

    drawPanel(rightX, topY, rightWidth, 206, "F407 SENSOR HUB", cyan);
    drawLine(rightX + 22, topY + 64, "HEARTBEAT", boolText(mcuStatus_.heartbeatSeen), mcuStatus_.heartbeatSeen ? green : yellow);
    drawLine(rightX + 22, topY + 92, "IMU", boolText(mcuStatus_.imuSeen), mcuStatus_.imuSeen ? green : yellow);
    drawLine(rightX + 22, topY + 120, "STATUS", intValue(mcuStatus_.statusFlags), white);
    drawLine(rightX + 22, topY + 148, "ACCEL", fixedValue(outdoor::protocol::accelMgToG(imu.accelXMg), 2) + "," +
                  fixedValue(outdoor::protocol::accelMgToG(imu.accelYMg), 2) + "," +
                  fixedValue(outdoor::protocol::accelMgToG(imu.accelZMg), 2),
             white);
    drawLine(rightX + 22, topY + 176, "GYRO", fixedValue(outdoor::protocol::gyroMdpsToDps(imu.gyroXMdps), 1) + "," +
                  fixedValue(outdoor::protocol::gyroMdpsToDps(imu.gyroYMdps), 1) + "," +
                  fixedValue(outdoor::protocol::gyroMdpsToDps(imu.gyroZMdps), 1),
             white);

    drawPanel(rightX, midY, rightWidth, 102, "AI LOCAL AGENT", {190, 135, 255});
    drawLine(rightX + 22, midY + 56, "STATE", "PLANNED", yellow);
    drawLine(rightX + 22, midY + 82, "MODE", "DASHBOARD ONLY", muted);

    drawPanel(rightX, bottomY, rightWidth, 98, "MP157 BOARD IMU", yellow);
    drawLine(rightX + 22, bottomY + 50, "SEEN", boolText(boardImuStatus_.seen), boardImuStatus_.seen ? green : yellow);
    drawLine(rightX + 22, bottomY + 76, "ACCEL", fixedValue(boardImuStatus_.accelXG, 2) + "," +
                  fixedValue(boardImuStatus_.accelYG, 2) + "," +
                  fixedValue(boardImuStatus_.accelZG, 2),
             white);

    canvas.fillRect(0, canvas.height() - 36, canvas.width(), 36, {10, 26, 51});
    canvas.drawText(32, canvas.height() - 24, "APP: OUTDOOR-AGENT / OUTPUT: FBDEV " + framebufferDevice_, muted, 2);
    outdoor::log::Logger::info("Dashboard rendered to framebuffer: " + framebufferDevice_);
    return true;
#endif
}

std::string DashboardService::render() const
{
    const auto& fix = gnssStatus_.fix;
    const auto& imu = mcuStatus_.imuSample;
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << "outdoor-agent\n"
           << "=============\n"
           << "mode: field dashboard\n"
           << "ai_agent_state: planned\n\n"
           << "[GNSS / u-blox M10]\n"
           << "source: " << gnssStatus_.source << "\n"
           << "seen: " << (gnssStatus_.seen ? "true" : "false") << "\n"
           << "valid_sentences: " << gnssStatus_.validSentenceCount << "\n"
           << "skipped_sentences: " << gnssStatus_.skippedSentenceCount << "\n"
           << "last_sentence_type: " << gnssStatus_.lastSentenceType << "\n"
           << "fix_valid: " << (fix.valid ? "true" : "false") << "\n"
           << std::setprecision(6)
           << "latitude_deg: " << fix.latitudeDegrees << "\n"
           << "longitude_deg: " << fix.longitudeDegrees << "\n"
           << std::setprecision(3)
           << "altitude_m: " << fix.altitudeMeters << "\n"
           << "speed_kmh: " << fix.speedKmh << "\n"
           << "speed_knots: " << fix.speedKnots << "\n"
           << "course_deg: " << fix.courseDegrees << "\n"
           << "satellites_used: " << fix.satelliteCount << "\n"
           << "satellites_in_view: " << fix.satellitesInView << "\n"
           << "fix_quality: " << fix.fixQuality << "\n"
           << "fix_type: " << fix.fixType << "\n"
           << "hdop: " << fix.hdop << "\n"
           << "pdop: " << fix.pdop << "\n"
           << "vdop: " << fix.vdop << "\n"
           << "utc_time: " << fix.utcTime << "\n";
    if (!gnssStatus_.lastError.empty()) {
        stream << "last_error: " << gnssStatus_.lastError << "\n";
    }

    stream << "\n[F407 Sensor Hub]\n"
           << "heartbeat_seen: " << (mcuStatus_.heartbeatSeen ? "true" : "false") << "\n"
           << "imu_seen: " << (mcuStatus_.imuSeen ? "true" : "false") << "\n"
           << "status_flags: " << mcuStatus_.statusFlags << "\n"
           << "last_frame_type: " << mcuStatus_.lastFrameType << "\n"
           << "accel_g: "
           << outdoor::protocol::accelMgToG(imu.accelXMg) << ", "
           << outdoor::protocol::accelMgToG(imu.accelYMg) << ", "
           << outdoor::protocol::accelMgToG(imu.accelZMg) << "\n"
           << "gyro_dps: "
           << outdoor::protocol::gyroMdpsToDps(imu.gyroXMdps) << ", "
           << outdoor::protocol::gyroMdpsToDps(imu.gyroYMdps) << ", "
           << outdoor::protocol::gyroMdpsToDps(imu.gyroZMdps) << "\n";

    stream << "\n[MP157 Board IMU]\n"
           << "enabled: " << (boardImuStatus_.enabled ? "true" : "false") << "\n"
           << "seen: " << (boardImuStatus_.seen ? "true" : "false") << "\n"
           << "source: " << boardImuStatus_.source << "\n"
           << "accel_g: " << boardImuStatus_.accelXG << ", "
           << boardImuStatus_.accelYG << ", " << boardImuStatus_.accelZG << "\n"
           << "gyro_dps: " << boardImuStatus_.gyroXDps << ", "
           << boardImuStatus_.gyroYDps << ", " << boardImuStatus_.gyroZDps << "\n"
           << "temperature_celsius: " << boardImuStatus_.temperatureCelsius << "\n";

    stream << "\n[AI Local Agent]\n"
           << "state: planned\n"
           << "current_view: dashboard_only\n"
           << "next_step: local deployment and interaction layer\n";

    return stream.str();
}

} // namespace outdoor::services
