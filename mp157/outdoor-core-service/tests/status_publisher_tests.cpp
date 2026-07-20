#include "ipc/status_publisher.h"

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
        / ("outdoor_status_publisher_test_" + std::to_string(unique))
        / "runtime_status.json";

    outdoor::runtime::RuntimeStatus status;
    status.storageEnabled = true;
    status.statusSocketEnabled = true;
    status.statusSocketPath = "runtime/status.sock";
    status.storageLogRotationFailureCount = 3U;
    status.storageLogWriteFailureCount = 2U;
    status.storageLogLastError = "rotation failed for \"outdoor.log\"";
    status.storageLastError = status.storageLogLastError;
    status.mcuStatus.diagnostics.seen = true;
    status.mcuStatus.diagnostics.schemaVersion = 1U;
    status.mcuStatus.diagnostics.uart4RxDropCount = 7U;

    outdoor::ipc::StatusPublisher publisher(output.string());
    std::string error;
    expect(publisher.publish(status, error), "status publisher should write JSON: " + error);

    const std::string json = readText(output);
    expect(json == outdoor::ipc::StatusPublisher::serialize(status),
           "file publisher and in-memory serialization should be identical");
    expect(json.find("\"status_socket_enabled\": true") != std::string::npos,
           "status JSON should report the enabled Unix socket");
    expect(json.find("\"status_socket_path\": \"runtime/status.sock\"") != std::string::npos,
           "status JSON should report the Unix socket path");
    expect(json.find("\"log_rotation_failure_count\": 3") != std::string::npos,
           "status JSON should contain the non-zero rotation failure count");
    expect(json.find("\"log_write_failure_count\": 2") != std::string::npos,
           "status JSON should contain the non-zero write failure count");
    expect(json.find("\"log_last_error\": \"rotation failed for \\\"outdoor.log\\\"\"")
               != std::string::npos,
           "status JSON should escape the logger error text");
    expect(json.find("\"last_error\": \"rotation failed for \\\"outdoor.log\\\"\"")
               != std::string::npos,
           "storage aggregate error should expose the logger failure");
    expect(json.find("\"schema_version\": 1") != std::string::npos,
           "status JSON should contain the diagnostics schema version");
    expect(json.find("\"uart4_rx_drop_count\": 7") != std::string::npos,
           "status JSON should contain the UART4 RX drop count");

    fs::remove_all(output.parent_path());
    std::cout << "status_publisher_tests passed\n";
    return 0;
}
