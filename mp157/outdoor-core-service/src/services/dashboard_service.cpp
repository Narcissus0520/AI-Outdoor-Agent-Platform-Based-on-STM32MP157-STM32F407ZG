#include "services/dashboard_service.h"

#include "log/logger.h"
#include "protocol/imu_payload.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
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
constexpr double kPi = 3.14159265358979323846;

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

std::string fixedValue(double value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

double clampToRange(double value, double minValue, double maxValue)
{
    return std::max(minValue, std::min(value, maxValue));
}

Rgb mixColor(Rgb a, Rgb b, double t)
{
    const double clamped = clampToRange(t, 0.0, 1.0);
    return {
        static_cast<std::uint8_t>(a.r + (b.r - a.r) * clamped),
        static_cast<std::uint8_t>(a.g + (b.g - a.g) * clamped),
        static_cast<std::uint8_t>(a.b + (b.b - a.b) * clamped),
    };
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

    void drawRect(int x, int y, int width, int height, Rgb color)
    {
        drawLine(x, y, x + width - 1, y, color);
        drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
        drawLine(x, y, x, y + height - 1, color);
        drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
    }

    void drawLine(int x0, int y0, int x1, int y1, Rgb color)
    {
        const int dx = std::abs(x1 - x0);
        const int sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0);
        const int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            setPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    void drawThickLine(int x0, int y0, int x1, int y1, Rgb color, int thickness)
    {
        const int radius = std::max(1, thickness / 2);
        for (int offset = -radius; offset <= radius; ++offset) {
            drawLine(x0 + offset, y0, x1 + offset, y1, color);
            drawLine(x0, y0 + offset, x1, y1 + offset, color);
        }
    }

    void fillCircle(int centerX, int centerY, int radius, Rgb color)
    {
        const int radiusSquared = radius * radius;
        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                if (x * x + y * y <= radiusSquared) {
                    setPixel(centerX + x, centerY + y, color);
                }
            }
        }
    }

    void drawCircle(int centerX, int centerY, int radius, Rgb color)
    {
        int x = radius;
        int y = 0;
        int err = 0;
        while (x >= y) {
            setPixel(centerX + x, centerY + y, color);
            setPixel(centerX + y, centerY + x, color);
            setPixel(centerX - y, centerY + x, color);
            setPixel(centerX - x, centerY + y, color);
            setPixel(centerX - x, centerY - y, color);
            setPixel(centerX - y, centerY - x, color);
            setPixel(centerX + y, centerY - x, color);
            setPixel(centerX + x, centerY - y, color);
            if (err <= 0) {
                ++y;
                err += 2 * y + 1;
            }
            if (err > 0) {
                --x;
                err -= 2 * x + 1;
            }
        }
    }

    void drawArc(int centerX, int centerY, int radius, double startDeg, double endDeg, Rgb color, int thickness)
    {
        const int steps = std::max(20, static_cast<int>(std::abs(endDeg - startDeg) * radius / 90.0));
        int lastX = centerX + static_cast<int>(std::cos(startDeg * kPi / 180.0) * radius);
        int lastY = centerY + static_cast<int>(std::sin(startDeg * kPi / 180.0) * radius);
        for (int step = 1; step <= steps; ++step) {
            const double t = static_cast<double>(step) / steps;
            const double angle = (startDeg + (endDeg - startDeg) * t) * kPi / 180.0;
            const int x = centerX + static_cast<int>(std::cos(angle) * radius);
            const int y = centerY + static_cast<int>(std::sin(angle) * radius);
            drawThickLine(lastX, lastY, x, y, color, thickness);
            lastX = x;
            lastY = y;
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
    const Rgb bg {3, 8, 19};
    const Rgb grid {9, 34, 62};
    const Rgb panel {8, 25, 49};
    const Rgb panelDeep {5, 17, 36};
    const Rgb border {30, 91, 145};
    const Rgb borderDim {15, 54, 93};
    const Rgb cyan {20, 206, 255};
    const Rgb cyanSoft {68, 178, 255};
    const Rgb blue {22, 96, 226};
    const Rgb green {45, 226, 157};
    const Rgb yellow {255, 205, 75};
    const Rgb white {232, 240, 252};
    const Rgb muted {134, 160, 191};
    const Rgb dim {57, 83, 118};

    const int screenWidth = canvas.width();
    const int screenHeight = canvas.height();
    const int sidebarWidth = 132;
    const int topHeight = 68;
    const int bottomHeight = 42;
    const int contentX = sidebarWidth + 8;
    const int contentY = topHeight + 8;
    const int contentRight = screenWidth - 10;

    canvas.fill(bg);
    for (int y = 0; y < screenHeight; y += 24) {
        canvas.drawLine(0, y, screenWidth, y, grid);
    }
    for (int x = 0; x < screenWidth; x += 32) {
        canvas.drawLine(x, 0, x, screenHeight, {5, 24, 45});
    }

    auto drawPanel = [&](int x, int y, int width, int height, std::string_view title, Rgb accent) {
        canvas.fillRect(x, y, width, height, panelDeep);
        canvas.fillRect(x + 2, y + 2, width - 4, height - 4, panel);
        canvas.drawRect(x, y, width, height, border);
        canvas.drawLine(x + 12, y + 32, x + width - 12, y + 32, borderDim);
        canvas.fillRect(x + 1, y + 1, width - 2, 3, accent);
        canvas.drawText(x + 16, y + 10, title, white, 2);
    };
    auto drawSmallMetric = [&](int x, int y, std::string_view label, std::string_view value, Rgb valueColor) {
        canvas.drawText(x, y, label, muted, 1);
        canvas.drawText(x, y + 14, value, valueColor, 2);
    };
    auto drawValueLine = [&](int x, int y, std::string_view label, std::string_view value, Rgb valueColor) {
        canvas.drawText(x, y, label, muted, 1);
        canvas.drawText(x + 76, y - 4, value, valueColor, 2);
    };

    canvas.fillRect(6, 6, sidebarWidth - 10, 58, {7, 20, 42});
    canvas.drawRect(6, 6, sidebarWidth - 10, 58, borderDim);
    canvas.drawThickLine(23, 46, 39, 20, cyan, 4);
    canvas.drawThickLine(39, 20, 55, 46, cyanSoft, 4);
    canvas.drawThickLine(31, 46, 49, 28, blue, 4);
    canvas.drawLine(20, 50, 70, 50, cyanSoft);
    canvas.drawText(76, 18, "OUTDOOR", white, 2);
    canvas.drawText(76, 40, "AGENT", cyan, 2);

    canvas.fillRect(8, 78, sidebarWidth - 16, screenHeight - 130, {5, 16, 35});
    canvas.drawRect(8, 78, sidebarWidth - 16, screenHeight - 130, borderDim);
    const std::array<std::string_view, 7> navItems {"DASHBOARD", "ENV MON", "LOCATION", "DATA LOG", "ALERTS", "DEVICE", "SYSTEM"};
    for (std::size_t index = 0; index < navItems.size(); ++index) {
        const int y = 90 + static_cast<int>(index) * 38;
        const bool active = index == 0;
        if (active) {
            canvas.fillRect(10, y - 4, sidebarWidth - 20, 30, {9, 51, 88});
            canvas.fillRect(10, y - 4, 3, 30, cyan);
        }
        canvas.drawCircle(28, y + 10, 7, active ? cyan : muted);
        canvas.drawText(45, y + 4, navItems[index], active ? cyan : muted, 1);
    }
    canvas.drawLine(46, 424, 73, 424, border);
    canvas.drawRect(54, 356, 14, 68, dim);
    canvas.drawRect(42, 332, 38, 24, muted);
    canvas.drawLine(48, 365, 25, 395, border);
    canvas.drawLine(74, 365, 96, 395, border);
    canvas.drawCircle(61, 432, 26, border);
    canvas.drawCircle(61, 432, 14, blue);
    canvas.drawText(30, 470, "SENSOR", muted, 1);
    canvas.drawText(34, 484, "TOWER", cyanSoft, 1);

    canvas.fillRect(contentX, 8, contentRight - contentX, 56, {5, 16, 35});
    canvas.drawRect(contentX, 8, contentRight - contentX, 56, borderDim);
    canvas.drawText(contentX + 28, 22, fix.utcTime.empty() ? "TIME --:--:--" : "UTC " + fix.utcTime, cyanSoft, 2);
    canvas.drawText(contentX + 170, 22, "FIELD DASHBOARD", white, 2);
    canvas.drawText(contentX + 360, 22, "WEATHER CLOUDY 26C", muted, 1);
    canvas.drawText(contentX + 520, 22, "AIR 32", green, 1);
    canvas.drawText(contentRight - 220, 22, "4G   BT   BAT 88%", white, 1);
    canvas.fillRect(contentRight - 96, 19, 78, 24, {13, 31, 49});
    canvas.drawRect(contentRight - 96, 19, 78, 24, borderDim);
    canvas.fillCircle(contentRight - 82, 31, 4, green);
    canvas.drawText(contentRight - 70, 26, "ONLINE", white, 1);

    const int compassX = contentX;
    const int compassY = contentY;
    const int compassW = 234;
    const int topCardH = 300;
    const int speedX = compassX + compassW + 10;
    const int speedW = 326;
    const int locationX = speedX + speedW + 10;
    const int locationW = contentRight - locationX;
    drawPanel(compassX, compassY, compassW, topCardH, "DIRECTION", cyan);
    const int compassCx = compassX + compassW / 2;
    const int compassCy = compassY + 132;
    canvas.drawCircle(compassCx, compassCy, 82, borderDim);
    canvas.drawCircle(compassCx, compassCy, 66, dim);
    canvas.drawCircle(compassCx, compassCy, 36, {10, 45, 75});
    for (int tick = 0; tick < 72; ++tick) {
        const double deg = tick * 5.0;
        const double rad = (deg - 90.0) * kPi / 180.0;
        const int outer = 82;
        const int inner = (tick % 6 == 0) ? 68 : 74;
        canvas.drawLine(compassCx + static_cast<int>(std::cos(rad) * inner),
                        compassCy + static_cast<int>(std::sin(rad) * inner),
                        compassCx + static_cast<int>(std::cos(rad) * outer),
                        compassCy + static_cast<int>(std::sin(rad) * outer),
                        tick % 6 == 0 ? muted : dim);
    }
    canvas.drawText(compassCx - 5, compassCy - 92, "N", white, 1);
    canvas.drawText(compassCx + 86, compassCy - 4, "E", white, 1);
    canvas.drawText(compassCx - 92, compassCy - 4, "W", white, 1);
    canvas.drawText(compassCx - 5, compassCy + 84, "S", muted, 1);
    const double course = fix.courseDegrees == 0.0 ? 62.3 : fix.courseDegrees;
    const double courseRad = course * kPi / 180.0;
    const int needleX = compassCx + static_cast<int>(std::sin(courseRad) * 62.0);
    const int needleY = compassCy - static_cast<int>(std::cos(courseRad) * 62.0);
    canvas.drawThickLine(compassCx, compassCy, needleX, needleY, cyan, 9);
    canvas.drawThickLine(compassCx, compassCy, compassCx - (needleX - compassCx) / 3, compassCy - (needleY - compassCy) / 3, blue, 6);
    canvas.fillCircle(compassCx, compassCy, 6, white);
    canvas.drawText(compassX + 72, compassY + 214, "NE 62", white, 4);
    canvas.drawText(compassX + 98, compassY + 254, "NORTH EAST", muted, 1);
    canvas.fillRect(compassX + 10, compassY + topCardH - 52, compassW - 20, 42, {8, 29, 55});
    canvas.drawRect(compassX + 10, compassY + topCardH - 52, compassW - 20, 42, borderDim);
    drawSmallMetric(compassX + 22, compassY + topCardH - 42, "MAG", "48.2UT", cyanSoft);
    drawSmallMetric(compassX + 95, compassY + topCardH - 42, "AZIMUTH", fixedValue(course, 1), white);
    drawSmallMetric(compassX + 172, compassY + topCardH - 42, "PITCH", fixedValue(boardImuStatus_.accelXG * 2.0, 1), white);

    drawPanel(speedX, compassY, speedW, topCardH, "SPEED", cyan);
    const int speedCx = speedX + speedW / 2;
    const int speedCy = compassY + 172;
    const double speedKmh = fix.speedKmh > 0.0 ? fix.speedKmh : 28.6;
    const double speedPercent = clampToRange(speedKmh / 100.0, 0.0, 1.0);
    canvas.drawCircle(speedCx, speedCy, 122, borderDim);
    canvas.drawArc(speedCx, speedCy, 124, 150, 390, {19, 45, 81}, 10);
    canvas.drawArc(speedCx, speedCy, 124, 150, 150 + 240 * speedPercent, cyan, 12);
    canvas.drawArc(speedCx, speedCy, 102, 150, 390, dim, 2);
    for (int tick = 0; tick <= 50; ++tick) {
        const double angle = (150.0 + tick * 240.0 / 50.0) * kPi / 180.0;
        const int outer = 111;
        const int inner = (tick % 5 == 0) ? 94 : 102;
        canvas.drawLine(speedCx + static_cast<int>(std::cos(angle) * inner),
                        speedCy + static_cast<int>(std::sin(angle) * inner),
                        speedCx + static_cast<int>(std::cos(angle) * outer),
                        speedCy + static_cast<int>(std::sin(angle) * outer),
                        tick % 5 == 0 ? cyanSoft : dim);
    }
    for (int label = 0; label <= 5; ++label) {
        const int value = label * 20;
        const double angle = (150.0 + value * 240.0 / 100.0) * kPi / 180.0;
        canvas.drawText(speedCx + static_cast<int>(std::cos(angle) * 78) - 10,
                        speedCy + static_cast<int>(std::sin(angle) * 78) - 8,
                        std::to_string(value), white, 1);
    }
    const double needleAngle = (150.0 + 240.0 * speedPercent) * kPi / 180.0;
    canvas.drawThickLine(speedCx, speedCy,
                         speedCx + static_cast<int>(std::cos(needleAngle) * 78),
                         speedCy + static_cast<int>(std::sin(needleAngle) * 78),
                         cyan, 5);
    canvas.drawText(speedCx - 46, speedCy - 12, fixedValue(speedKmh, 1), white, 5);
    canvas.drawText(speedCx - 22, speedCy + 42, "KM/H", muted, 2);
    canvas.fillRect(speedX + 34, compassY + topCardH - 54, speedW - 68, 44, {8, 29, 55});
    canvas.drawRect(speedX + 34, compassY + topCardH - 54, speedW - 68, 44, borderDim);
    drawSmallMetric(speedX + 52, compassY + topCardH - 42, "TRIP", "12.84KM", cyan);
    drawSmallMetric(speedX + 142, compassY + topCardH - 42, "MAX", "58.7KMH", cyanSoft);
    drawSmallMetric(speedX + 236, compassY + topCardH - 42, "AVG", "23.1KMH", cyanSoft);

    drawPanel(locationX, compassY, locationW, topCardH, "LOCATION", cyan);
    drawValueLine(locationX + 18, compassY + 48, "LAT", fixedValue(fix.latitudeDegrees, 4) + " N", white);
    drawValueLine(locationX + locationW / 2 + 6, compassY + 48, "LON", fixedValue(fix.longitudeDegrees, 4) + " E", white);
    drawValueLine(locationX + 18, compassY + 86, "ALT", fixedValue(fix.altitudeMeters, 1) + " M", white);
    drawValueLine(locationX + locationW / 2 + 6, compassY + 86, "HDOP", fixedValue(fix.hdop, 1) + " M", white);
    const int mapX = locationX + 10;
    const int mapY = compassY + 126;
    const int mapW = locationW - 20;
    const int mapH = 126;
    canvas.fillRect(mapX, mapY, mapW, mapH, {6, 22, 42});
    canvas.drawRect(mapX, mapY, mapW, mapH, borderDim);
    for (int i = 0; i < 6; ++i) {
        canvas.drawLine(mapX + i * mapW / 5, mapY, mapX + (i + 1) * mapW / 5 - 26, mapY + mapH, dim);
        canvas.drawLine(mapX, mapY + i * mapH / 5, mapX + mapW, mapY + (i * mapH / 5) - 18, {9, 48, 82});
    }
    canvas.drawThickLine(mapX + 18, mapY + 92, mapX + 122, mapY + 62, cyan, 3);
    canvas.drawThickLine(mapX + 122, mapY + 62, mapX + 174, mapY + 28, blue, 3);
    const int pinX = mapX + mapW / 2 + 36;
    const int pinY = mapY + 58;
    canvas.fillCircle(pinX, pinY, 17, cyan);
    canvas.fillCircle(pinX, pinY, 7, blue);
    canvas.drawThickLine(pinX, pinY + 12, pinX, pinY + 44, cyan, 5);
    canvas.drawCircle(pinX, pinY + 48, 25, cyan);
    canvas.drawText(locationX + 22, compassY + topCardH - 26, "FIX NORMAL", green, 1);
    canvas.drawText(locationX + locationW - 116, compassY + topCardH - 26, fix.utcTime.empty() ? "UPDATE --" : "UPDATE " + fix.utcTime, muted, 1);

    const int envY = compassY + topCardH + 14;
    const int tempX = contentX;
    const int tempW = 326;
    const int luxX = tempX + tempW + 10;
    const int luxW = contentRight - luxX;
    drawPanel(tempX, envY, tempW, 152, "TEMPERATURE", cyan);
    const double tempC = boardImuStatus_.seen ? boardImuStatus_.temperatureCelsius : 26.4;
    canvas.drawArc(tempX + 76, envY + 82, 54, 130, 410, {18, 45, 80}, 10);
    canvas.drawArc(tempX + 76, envY + 82, 54, 130, 130 + 280 * clampToRange((tempC - 10.0) / 35.0, 0.0, 1.0), cyan, 11);
    canvas.drawText(tempX + 48, envY + 66, fixedValue(tempC, 1), white, 4);
    canvas.drawText(tempX + 82, envY + 104, "C", muted, 2);
    drawValueLine(tempX + 168, envY + 52, "BODY", fixedValue(tempC + 0.4, 1) + "C", white);
    drawValueLine(tempX + 168, envY + 82, "MAX", fixedValue(tempC + 2.1, 1) + "C", white);
    drawValueLine(tempX + 168, envY + 112, "MIN", fixedValue(tempC - 4.3, 1) + "C", white);
    const int graphX = tempX + 20;
    const int graphY = envY + 124;
    const std::array<int, 13> tempGraph {8, 10, 6, 9, 14, 18, 30, 19, 21, 25, 16, 9, 11};
    for (std::size_t i = 1; i < tempGraph.size(); ++i) {
        const int x0 = graphX + static_cast<int>((i - 1) * 22);
        const int y0 = graphY - tempGraph[i - 1];
        const int x1 = graphX + static_cast<int>(i * 22);
        const int y1 = graphY - tempGraph[i];
        canvas.drawLine(x0, y0, x1, y1, cyanSoft);
        canvas.fillRect(x1, y1, 2, tempGraph[i], {8, 57, 92});
    }

    drawPanel(luxX, envY, luxW, 152, "LIGHT INTENSITY", yellow);
    const int luxValue = 18450;
    canvas.drawArc(luxX + 76, envY + 86, 54, 130, 410, {18, 45, 80}, 10);
    canvas.drawArc(luxX + 76, envY + 86, 54, 130, 130 + 280 * clampToRange(luxValue / 30000.0, 0.0, 1.0), yellow, 11);
    canvas.drawText(luxX + 43, envY + 70, std::to_string(luxValue), white, 3);
    canvas.drawText(luxX + 82, envY + 104, "LUX", muted, 1);
    const int barX = luxX + 164;
    const int barY = envY + 56;
    for (int i = 0; i < 38; ++i) {
        const double t = static_cast<double>(i) / 37.0;
        const int barH = 12 + static_cast<int>(32 * t);
        const Rgb color = t < 0.72 ? mixColor(blue, cyan, t / 0.72) : mixColor(cyan, yellow, (t - 0.72) / 0.28);
        canvas.fillRect(barX + i * 9, barY + 48 - barH, 5, barH, color);
    }
    drawSmallMetric(luxX + 168, envY + 108, "MIN", "320LUX", white);
    drawSmallMetric(luxX + 300, envY + 108, "AVG", "12680LUX", white);
    drawSmallMetric(luxX + 450, envY + 108, "MAX", "25630LUX", white);
    canvas.drawText(luxX + luxW - 96, envY + 14, "LEVEL STRONG", yellow, 1);

    canvas.fillRect(8, screenHeight - bottomHeight, screenWidth - 16, bottomHeight - 6, {6, 18, 38});
    canvas.drawRect(8, screenHeight - bottomHeight, screenWidth - 16, bottomHeight - 6, borderDim);
    const std::array<std::string_view, 7> footerLabels {"DEVICE", "UPTIME", "DATA", "STORAGE", "SIGNAL", "POWER", "FW"};
    const std::array<std::string_view, 7> footerValues {"OUTDOOR-2026", "03:45:21", "NORMAL", "68%", "-75DBM", "12.6V", "V2.1.8"};
    for (std::size_t i = 0; i < footerLabels.size(); ++i) {
        const int x = 26 + static_cast<int>(i) * 142;
        canvas.drawText(x, screenHeight - 34, footerLabels[i], muted, 1);
        canvas.drawText(x, screenHeight - 20, footerValues[i], i == 2 ? green : white, 1);
        if (i > 0) {
            canvas.drawLine(x - 18, screenHeight - 34, x - 18, screenHeight - 10, borderDim);
        }
    }
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
