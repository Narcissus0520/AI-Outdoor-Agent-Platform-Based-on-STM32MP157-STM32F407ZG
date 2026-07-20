cmake_minimum_required(VERSION 3.16)

project(outdoor_compass_calibrator LANGUAGES CXX)

include(CTest)

add_library(compass_calibration
    src/compass_calibrator.cpp
)

target_include_directories(compass_calibration
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(compass_calibration PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(compass_calibration PRIVATE /W4 /permissive-)
else()
    target_compile_options(compass_calibration PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(compass_calibrator
    src/main.cpp
)

target_link_libraries(compass_calibrator PRIVATE compass_calibration)

if(MSVC)
    target_compile_options(compass_calibrator PRIVATE /W4 /permissive-)
else()
    target_compile_options(compass_calibrator PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(compass_calibrator_tests
        tests/compass_calibrator_test.cpp
    )
    target_link_libraries(compass_calibrator_tests PRIVATE compass_calibration)
    if(MSVC)
        target_compile_options(compass_calibrator_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(compass_calibrator_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME compass_calibrator_tests COMMAND compass_calibrator_tests)
    add_test(NAME compass_calibrator_cli_help COMMAND compass_calibrator --help)
    add_test(
        NAME compass_calibrator_cli_fixture
        COMMAND compass_calibrator
            --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/magnetometer_sphere.csv
            --min-samples 20
            --output ${CMAKE_CURRENT_BINARY_DIR}/compass_candidate_smoke.conf
    )
endif()
