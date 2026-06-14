cmake_minimum_required(VERSION 3.16)

project(sensor_hub_mock
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sensor_hub_mock
    src/drivers/icm42688_driver.cpp
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
