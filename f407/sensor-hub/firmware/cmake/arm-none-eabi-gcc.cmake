set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_TOOLCHAIN_ROOT "" CACHE PATH "GNU Arm Embedded Toolchain root directory")

set(_arm_toolchain_hints)
if(ARM_TOOLCHAIN_ROOT)
    list(APPEND _arm_toolchain_hints "${ARM_TOOLCHAIN_ROOT}")
endif()
if(DEFINED ENV{ARM_TOOLCHAIN_ROOT})
    list(APPEND _arm_toolchain_hints "$ENV{ARM_TOOLCHAIN_ROOT}")
endif()
if(WIN32)
    file(GLOB _arm_windows_roots LIST_DIRECTORIES true
        "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*"
        "C:/Program Files/Arm GNU Toolchain arm-none-eabi/*"
    )
    list(APPEND _arm_toolchain_hints
        ${_arm_windows_roots}
        "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1"
        "C:/Program Files/Arm GNU Toolchain arm-none-eabi/14.2 rel1"
    )
endif()

find_program(ARM_GCC
    NAMES arm-none-eabi-gcc
    HINTS ${_arm_toolchain_hints}
    PATH_SUFFIXES bin
    REQUIRED
)

get_filename_component(ARM_TOOLCHAIN_BIN_DIR "${ARM_GCC}" DIRECTORY)

set(CMAKE_C_COMPILER "${ARM_GCC}" CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${ARM_GCC}" CACHE FILEPATH "" FORCE)
set(CMAKE_AR "${ARM_TOOLCHAIN_BIN_DIR}/arm-none-eabi-ar${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "" FORCE)
set(CMAKE_OBJCOPY "${ARM_TOOLCHAIN_BIN_DIR}/arm-none-eabi-objcopy${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "" FORCE)
set(CMAKE_SIZE "${ARM_TOOLCHAIN_BIN_DIR}/arm-none-eabi-size${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "" FORCE)

set(CMAKE_FIND_ROOT_PATH "${ARM_TOOLCHAIN_BIN_DIR}/..")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
