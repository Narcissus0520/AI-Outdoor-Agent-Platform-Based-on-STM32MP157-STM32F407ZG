cmake_minimum_required(VERSION 3.16)

project(sensor_hub_mock
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sensor_hub_mock
    src/protocol/mcu_frame_builder.cpp
    src/sensors/mock_imu_provider.cpp
)

target_include_directories(sensor_hub_mock
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_compile_options(sensor_hub_mock
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

enable_testing()

add_executable(sensor_hub_mock_tests
    tests/mock_imu_provider_test.cpp
)

target_link_libraries(sensor_hub_mock_tests
    PRIVATE
        sensor_hub_mock
)

add_test(NAME sensor_hub_mock_tests COMMAND sensor_hub_mock_tests)

add_executable(sensor_sample_filter_tests
    tests/sensor_sample_filter_test.cpp
    firmware/sensors/sensor_sample_filter_c.c
)

set_target_properties(sensor_sample_filter_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(sensor_sample_filter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

target_compile_options(sensor_sample_filter_tests
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_test(NAME sensor_sample_filter_tests COMMAND sensor_sample_filter_tests)

add_executable(icm42688_fifo_parser_tests
    tests/icm42688_fifo_parser_test.cpp
    firmware/sensors/icm42688_fifo_parser_c.c
    firmware/sensors/imu_timestamp_normalizer_c.c
)

set_target_properties(icm42688_fifo_parser_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(icm42688_fifo_parser_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

target_compile_options(icm42688_fifo_parser_tests
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_test(NAME icm42688_fifo_parser_tests COMMAND icm42688_fifo_parser_tests)

add_executable(icm42688_provider_tests
    tests/icm42688_provider_test.cpp
    firmware/sensors/icm42688_fifo_parser_c.c
    firmware/sensors/icm42688_provider_c.c
)

set_target_properties(icm42688_provider_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(icm42688_provider_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

target_compile_options(icm42688_provider_tests
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_test(NAME icm42688_provider_tests COMMAND icm42688_provider_tests)

add_executable(mmc5603_provider_tests
    tests/mmc5603_provider_test.cpp
    firmware/sensors/mmc5603_provider_c.c
)

set_target_properties(mmc5603_provider_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(mmc5603_provider_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

target_compile_options(mmc5603_provider_tests
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_test(NAME mmc5603_provider_tests COMMAND mmc5603_provider_tests)

add_executable(uart_tx_queue_tests
    tests/uart_tx_queue_test.cpp
    firmware/bsp/uart_tx_queue_c.c
)

set_target_properties(uart_tx_queue_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(uart_tx_queue_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

target_compile_options(uart_tx_queue_tests
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_test(NAME uart_tx_queue_tests COMMAND uart_tx_queue_tests)

add_executable(mcu_frame_builder_tests
    tests/mcu_frame_builder_test.cpp
    firmware/protocol/crc16_modbus_c.c
    firmware/protocol/mcu_frame_builder_c.c
)

set_target_properties(mcu_frame_builder_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(mcu_frame_builder_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

target_compile_options(mcu_frame_builder_tests
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_test(NAME mcu_frame_builder_tests COMMAND mcu_frame_builder_tests)
