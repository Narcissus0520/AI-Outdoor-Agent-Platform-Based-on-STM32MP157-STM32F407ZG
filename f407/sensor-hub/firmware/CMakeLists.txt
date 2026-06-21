cmake_minimum_required(VERSION 3.20)

project(sensor_hub_firmware
    VERSION 0.1.0
    LANGUAGES C
)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "F407 firmware requires arm-none-eabi-gcc")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CUBE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/stm32cube")
set(HAL_ROOT "${CUBE_ROOT}/Drivers/STM32F4xx_HAL_Driver")
set(CMSIS_ROOT "${CUBE_ROOT}/Drivers/CMSIS")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F407ZGTx_FLASH.ld")

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(CUBE_CORE_SOURCES
    "${CUBE_ROOT}/Core/Src/main.c"
    "${CUBE_ROOT}/Core/Src/gpio.c"
    "${CUBE_ROOT}/Core/Src/i2c.c"
    "${CUBE_ROOT}/Core/Src/usart.c"
    "${CUBE_ROOT}/Core/Src/stm32f4xx_it.c"
    "${CUBE_ROOT}/Core/Src/stm32f4xx_hal_msp.c"
    "${CUBE_ROOT}/Core/Src/system_stm32f4xx.c"
)

set(HAL_SOURCES
    "${HAL_ROOT}/Src/stm32f4xx_hal.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_cortex.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_dma.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_dma_ex.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_exti.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_flash.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_flash_ex.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_flash_ramfunc.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_gpio.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_i2c.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_i2c_ex.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_pwr.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_pwr_ex.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_rcc.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_rcc_ex.c"
    "${HAL_ROOT}/Src/stm32f4xx_hal_uart.c"
)

add_executable(sensor_hub
    startup/startup_stm32f407xx.c
    platform/syscalls.c
    app/sensor_hub_app.c
    bsp/board_i2c.c
    bsp/board_icm42688_gpio.c
    bsp/board_tick.c
    bsp/board_uart.c
    bsp/stm32/board_i2c_stm32.c
    bsp/stm32/board_icm42688_gpio_stm32.c
    bsp/stm32/board_tick_stm32.c
    bsp/stm32/board_uart_stm32.c
    protocol/crc16_modbus_c.c
    protocol/mcu_command_decoder_c.c
    protocol/mcu_frame_builder_c.c
    sensors/icm42688_provider_c.c
    sensors/mock_imu_provider_c.c
    ${CUBE_CORE_SOURCES}
    ${HAL_SOURCES}
)

set_source_files_properties(
    startup/startup_stm32f407xx.c
    PROPERTIES COMPILE_OPTIONS -Wno-pedantic
)

set_source_files_properties(
    ${HAL_SOURCES}
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter
)

set_target_properties(sensor_hub PROPERTIES
    SUFFIX ".elf"
)

target_compile_definitions(sensor_hub PRIVATE
    USE_HAL_DRIVER
    STM32F407xx
)

target_include_directories(sensor_hub PRIVATE
    "${CUBE_ROOT}/Core/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${HAL_ROOT}/Inc"
    "${HAL_ROOT}/Inc/Legacy"
    "${CMSIS_ROOT}/Device/ST/STM32F4xx/Include"
    "${CMSIS_ROOT}/Include"
)

target_compile_options(sensor_hub PRIVATE
    ${CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>
    $<$<NOT:$<CONFIG:Debug>>:-Os>
)

target_link_options(sensor_hub PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/sensor_hub.map
)

target_link_libraries(sensor_hub PRIVATE
    m
    c
)

add_custom_command(TARGET sensor_hub POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex
            "$<TARGET_FILE:sensor_hub>"
            "${CMAKE_CURRENT_BINARY_DIR}/sensor_hub.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary
            "$<TARGET_FILE:sensor_hub>"
            "${CMAKE_CURRENT_BINARY_DIR}/sensor_hub.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:sensor_hub>"
    COMMENT "Generating HEX/BIN images and reporting firmware size"
)
