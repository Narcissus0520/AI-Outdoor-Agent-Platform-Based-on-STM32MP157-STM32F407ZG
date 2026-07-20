cmake_minimum_required(VERSION 3.16)

project(ai_outdoor_agent_platform
    VERSION 1.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OUTDOOR_CORE_SOURCES
    src/config/config_loader.cpp
    src/gnss/gnss_status.cpp
    src/gnss/nmea_parser.cpp
    src/input/file_replay_input.cpp
    src/ipc/status_publisher.cpp
    src/log/logger.cpp
    src/mcu/imu_payload_parser.cpp
    src/mcu/mcu_command.cpp
    src/mcu/mcu_frame_parser.cpp
    src/mcu/mcu_frame_stream_decoder.cpp
    src/mcu/mcu_protocol.cpp
    src/mcu/mcu_status.cpp
    src/navigation/compass_estimator.cpp
    src/runtime/runtime_manager.cpp
    src/runtime/runtime_status.cpp
    src/sensors/icm20608_char_reader.cpp
    src/sensors/icm20608_iio_reader.cpp
    src/services/dashboard_service.cpp
    src/services/gnss_replay_service.cpp
    src/services/gnss_serial_service.cpp
    src/services/icm20608_service.cpp
    src/services/mcu_mock_service.cpp
    src/services/mcu_serial_service.cpp
    src/storage/history_recorder.cpp
)

add_library(outdoor_core STATIC
    ${OUTDOOR_CORE_SOURCES}
)

target_include_directories(outdoor_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_compile_options(outdoor_core
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/FS>
)

add_executable(outdoor_core_runtime
    src/main.cpp
)

target_link_libraries(outdoor_core_runtime
    PRIVATE
        outdoor_core
)

enable_testing()

add_executable(mcu_protocol_tests
    tests/mcu_protocol_tests.cpp
)

target_link_libraries(mcu_protocol_tests
    PRIVATE
        outdoor_core
)

add_test(NAME mcu_protocol_tests COMMAND mcu_protocol_tests)

add_executable(mcu_frame_stream_decoder_tests
    tests/mcu_frame_stream_decoder_tests.cpp
)

target_link_libraries(mcu_frame_stream_decoder_tests
    PRIVATE
        outdoor_core
)

add_test(NAME mcu_frame_stream_decoder_tests COMMAND mcu_frame_stream_decoder_tests)

add_executable(icm20608_iio_reader_tests
    tests/icm20608_iio_reader_tests.cpp
)

target_link_libraries(icm20608_iio_reader_tests
    PRIVATE
        outdoor_core
)

add_test(NAME icm20608_iio_reader_tests COMMAND icm20608_iio_reader_tests)

add_executable(icm20608_char_reader_tests
    tests/icm20608_char_reader_tests.cpp
)

target_link_libraries(icm20608_char_reader_tests
    PRIVATE
        outdoor_core
)

add_test(NAME icm20608_char_reader_tests COMMAND icm20608_char_reader_tests)

add_executable(nmea_parser_tests
    tests/nmea_parser_tests.cpp
)

target_link_libraries(nmea_parser_tests
    PRIVATE
        outdoor_core
)

add_test(NAME nmea_parser_tests COMMAND nmea_parser_tests)

add_executable(runtime_manager_tests
    tests/runtime_manager_tests.cpp
)

target_link_libraries(runtime_manager_tests
    PRIVATE
        outdoor_core
)

add_test(NAME runtime_manager_tests COMMAND runtime_manager_tests)

add_executable(history_recorder_tests
    tests/history_recorder_tests.cpp
)

target_link_libraries(history_recorder_tests
    PRIVATE
        outdoor_core
)

add_test(NAME history_recorder_tests COMMAND history_recorder_tests)

add_executable(logger_tests
    tests/logger_tests.cpp
)

target_link_libraries(logger_tests
    PRIVATE
        outdoor_core
)

add_test(NAME logger_tests COMMAND logger_tests)

add_executable(status_publisher_tests
    tests/status_publisher_tests.cpp
)

target_link_libraries(status_publisher_tests
    PRIVATE
        outdoor_core
)

add_test(NAME status_publisher_tests COMMAND status_publisher_tests)

add_executable(compass_estimator_tests
    tests/compass_estimator_tests.cpp
)

target_link_libraries(compass_estimator_tests
    PRIVATE
        outdoor_core
)

add_test(NAME compass_estimator_tests COMMAND compass_estimator_tests)
