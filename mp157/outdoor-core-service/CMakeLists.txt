cmake_minimum_required(VERSION 3.16)

project(ai_outdoor_agent_platform
    VERSION 1.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(outdoor_core_runtime
    src/main.cpp
    src/config/config_loader.cpp
    src/gnss/nmea_parser.cpp
    src/input/file_replay_input.cpp
    src/ipc/status_publisher.cpp
    src/log/logger.cpp
    src/mcu/mcu_frame_parser.cpp
    src/mcu/mcu_protocol.cpp
    src/mcu/mcu_status.cpp
    src/runtime/runtime_manager.cpp
    src/runtime/runtime_status.cpp
    src/services/gnss_replay_service.cpp
    src/services/mcu_mock_service.cpp
)

target_include_directories(outdoor_core_runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(outdoor_core_runtime
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
