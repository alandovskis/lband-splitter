cmake_minimum_required(VERSION 3.16)

# Set the cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)

# Don't run the linker on compiler check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32f4_frequency_detector C ASM)

# Core CPU and FPU settings
set(CPU_PARAMETERS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard)

# Compiler flags
set(CMAKE_C_FLAGS "${CPU_PARAMETERS} -Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat=2 -Wformat-truncation -Wundef -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-usage --specs=nano.specs")

# Debug and release flags
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -ggdb -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0 -DNDEBUG")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CPU_PARAMETERS} -Wl,-Map=${PROJECT_NAME}.map")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")

# STM32 HAL definitions
add_compile_definitions(
    USE_HAL_DRIVER
    STM32F407xx
    HSE_VALUE=8000000
    HAL_RCC_MODULE_ENABLED
    HAL_GPIO_MODULE_ENABLED
    HAL_DMA_MODULE_ENABLED
    HAL_CORTEX_MODULE_ENABLED
    HAL_ADC_MODULE_ENABLED
    HAL_FLASH_MODULE_ENABLED
    HAL_PWR_MODULE_ENABLED
    HAL_TIM_MODULE_ENABLED
    HAL_UART_MODULE_ENABLED
)

# Check if STM32 HAL library is available
find_path(STM32_HAL_PATH
    NAMES Inc/stm32f4xx_hal.h
    PATHS
        /opt/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver
        /usr/local/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver
        $ENV{STM32_HAL_PATH}
    DOC "Path to STM32F4 HAL library"
)

if(NOT STM32_HAL_PATH)
    message(WARNING "STM32F4 HAL library not found. Firmware will not be built.")
    message(WARNING "Please install STM32CubeF4 or set STM32_HAL_PATH environment variable.")
    return()
endif()

message(STATUS "Found STM32F4 HAL at: ${STM32_HAL_PATH}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${STM32_HAL_PATH}/Inc
    ${STM32_HAL_PATH}/Inc/Legacy
)

# HAL source files (core components only)
set(STM32_HAL_SOURCES
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_rcc.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_rcc_ex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_gpio.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_dma.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_cortex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_adc.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_adc_ex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_flash.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_flash_ex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_pwr.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_pwr_ex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_tim.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_tim_ex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_uart.c
)

# Check if CMSIS is available
find_path(CMSIS_PATH
    NAMES Include/core_cm4.h
    PATHS
        /opt/STM32CubeF4/Drivers/CMSIS
        /usr/local/STM32CubeF4/Drivers/CMSIS
        $ENV{CMSIS_PATH}
    DOC "Path to CMSIS library"
)

if(NOT CMSIS_PATH)
    message(WARNING "CMSIS library not found. Using basic system definitions.")
else()
    message(STATUS "Found CMSIS at: ${CMSIS_PATH}")
    include_directories(${CMSIS_PATH}/Include)
    
    # CMSIS device files
    find_path(CMSIS_DEVICE_PATH
        NAMES Include/stm32f4xx.h
        PATHS
            /opt/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx
            /usr/local/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx
            $ENV{CMSIS_DEVICE_PATH}
        DOC "Path to CMSIS device files"
    )
    
    if(CMSIS_DEVICE_PATH)
        include_directories(${CMSIS_DEVICE_PATH}/Include)
        set(CMSIS_SOURCES ${CMSIS_DEVICE_PATH}/Source/Templates/system_stm32f4xx.c)
    endif()
endif()

# Project source files
set(SOURCES
    main.c
    frequency_detector.c
    uart_protocol.c
    stm32f4xx_it.c
    ${STM32_HAL_SOURCES}
    ${CMSIS_SOURCES}
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Linker script (if available)
find_file(LINKER_SCRIPT
    NAMES STM32F407VGTx_FLASH.ld STM32F407xx_FLASH.ld
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}
        /opt/STM32CubeF4/Projects/STM32F4-Discovery/Templates/SW4STM32/STM32F4-Discovery
        $ENV{STM32_LINKER_SCRIPTS}
    DOC "STM32F4 linker script"
)

if(LINKER_SCRIPT)
    message(STATUS "Found linker script: ${LINKER_SCRIPT}")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_DEPENDS ${LINKER_SCRIPT}
    )
    target_link_options(${PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})
else()
    message(WARNING "Linker script not found. Using default linker behavior.")
endif()

# Generate additional build outputs
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating HEX and BIN files, showing size information"
)

# Print build information
add_custom_target(info
    COMMAND ${CMAKE_OBJDUMP} -h -S ${PROJECT_NAME} > ${PROJECT_NAME}.list
    COMMAND ${SIZE} ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "Generating disassembly and size information"
)

# Flash target (requires openocd or st-flash)
add_custom_target(flash
    COMMAND st-flash write ${PROJECT_NAME}.bin 0x8000000
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing firmware to STM32F4"
)

# Alternative flash using openocd
add_custom_target(flash_openocd
    COMMAND openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg 
            -c "program ${PROJECT_NAME}.hex verify reset exit"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing firmware using OpenOCD"
)