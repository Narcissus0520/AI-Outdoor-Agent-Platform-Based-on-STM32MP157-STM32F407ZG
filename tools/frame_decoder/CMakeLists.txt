cmake_minimum_required(VERSION 3.16)

project(frame_decoder
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(frame_decoder
    src/main.cpp
)

target_include_directories(frame_decoder
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_compile_options(frame_decoder
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
