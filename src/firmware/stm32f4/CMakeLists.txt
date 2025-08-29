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

# Project source files for main firmware
set(FIRMWARE_SOURCES
    main.c
    frequency_detector.c
    uart_protocol.c
    port.c
    display_abstraction.c
    led_abstraction.c
    stm32f4xx_it.c
    ${STM32_HAL_SOURCES}
    ${CMSIS_SOURCES}
)

# Secure bootloader source files
set(BOOTLOADER_SOURCES
    bootloader_main.c
    secure_boot.c
    stm32f4xx_it.c
    ${STM32_HAL_SOURCES}
    ${CMSIS_SOURCES}
)

# Create the main firmware executable
add_executable(${PROJECT_NAME} ${FIRMWARE_SOURCES})

# Create the secure bootloader executable
add_executable(${PROJECT_NAME}_bootloader ${BOOTLOADER_SOURCES})

# Add secure boot definitions for bootloader
target_compile_definitions(${PROJECT_NAME}_bootloader PRIVATE
    SECURE_BOOT_ENABLE_DEBUG=1
    SECURE_BOOT_ENABLE_DEVELOPMENT_MODE=0
    SECURE_BOOT_ENABLE_ROLLBACK_PROTECTION=1
)

# Linker script (if available)
find_file(LINKER_SCRIPT
    NAMES STM32F407VGTx_FLASH.ld STM32F407xx_FLASH.ld
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}
        /opt/STM32CubeF4/Projects/STM32F4-Discovery/Templates/SW4STM32/STM32F4-Discovery
        $ENV{STM32_LINKER_SCRIPTS}
    DOC "STM32F4 linker script"
)

# Look for bootloader-specific linker script
find_file(BOOTLOADER_LINKER_SCRIPT
    NAMES STM32F407VGTx_BOOTLOADER.ld STM32F407xx_BOOTLOADER.ld
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}
        $ENV{STM32_LINKER_SCRIPTS}
    DOC "STM32F4 bootloader linker script"
)

if(LINKER_SCRIPT)
    message(STATUS "Found linker script: ${LINKER_SCRIPT}")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_DEPENDS ${LINKER_SCRIPT}
    )
    target_link_options(${PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})
    
    # Use bootloader linker script if available, otherwise use main script
    if(BOOTLOADER_LINKER_SCRIPT)
        message(STATUS "Found bootloader linker script: ${BOOTLOADER_LINKER_SCRIPT}")
        set_target_properties(${PROJECT_NAME}_bootloader PROPERTIES
            LINK_DEPENDS ${BOOTLOADER_LINKER_SCRIPT}
        )
        target_link_options(${PROJECT_NAME}_bootloader PRIVATE -T${BOOTLOADER_LINKER_SCRIPT})
    else()
        message(STATUS "Using main linker script for bootloader")
        set_target_properties(${PROJECT_NAME}_bootloader PROPERTIES
            LINK_DEPENDS ${LINKER_SCRIPT}
        )
        target_link_options(${PROJECT_NAME}_bootloader PRIVATE -T${LINKER_SCRIPT})
    endif()
else()
    message(WARNING "Linker script not found. Using default linker behavior.")
endif()

# Secure boot configuration variables
set(FIRMWARE_VERSION "1" CACHE STRING "Firmware version number")
set(ROLLBACK_COUNTER "1" CACHE STRING "Rollback protection counter")
set(SIGNING_KEY_PATH "" CACHE FILEPATH "Path to firmware signing private key")
set(PUBLIC_KEY_PATH "" CACHE FILEPATH "Path to firmware signing public key")

# Find Python and firmware signing script
find_package(Python3 COMPONENTS Interpreter)
set(FIRMWARE_SIGN_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/firmware_sign.py")
set(FIRMWARE_VERIFY_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/firmware_verify.py")

# Generate additional build outputs for main firmware
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating HEX and BIN files for main firmware, showing size information"
)

# Generate build outputs for bootloader
add_custom_command(TARGET ${PROJECT_NAME}_bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}_bootloader> ${PROJECT_NAME}_bootloader.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}_bootloader> ${PROJECT_NAME}_bootloader.bin
    COMMAND ${SIZE} $<TARGET_FILE:${PROJECT_NAME}_bootloader>
    COMMENT "Generating HEX and BIN files for bootloader, showing size information"
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

# =============================================================================
# Secure Boot Targets
# =============================================================================

# Generate signing keypair (only if Python is available)
if(Python3_FOUND AND EXISTS ${FIRMWARE_SIGN_SCRIPT})
    add_custom_target(generate_keys
        COMMAND ${Python3_EXECUTABLE} ${FIRMWARE_SIGN_SCRIPT} keygen
                --private-key firmware_signing_key.pem
                --public-key firmware_public_key.pem
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating Ed25519 signing keypair for secure boot"
    )

    # Sign firmware (only if signing key is provided)
    if(EXISTS "${SIGNING_KEY_PATH}")
        message(STATUS "Firmware signing enabled with key: ${SIGNING_KEY_PATH}")
        
        add_custom_target(sign_firmware
            COMMAND ${Python3_EXECUTABLE} ${FIRMWARE_SIGN_SCRIPT} sign
                    --firmware ${PROJECT_NAME}.bin
                    --private-key ${SIGNING_KEY_PATH}
                    --output ${PROJECT_NAME}_signed.bin
                    --version ${FIRMWARE_VERSION}
                    --rollback-counter ${ROLLBACK_COUNTER}
            DEPENDS ${PROJECT_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Signing firmware with secure boot header"
        )
        
        # Verify signed firmware (if public key is provided)
        if(EXISTS "${PUBLIC_KEY_PATH}")
            add_custom_target(verify_firmware
                COMMAND ${Python3_EXECUTABLE} ${FIRMWARE_VERIFY_SCRIPT} verify
                        --firmware ${PROJECT_NAME}_signed.bin
                        --public-key ${PUBLIC_KEY_PATH}
                DEPENDS sign_firmware
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Verifying signed firmware"
            )
        endif()
        
        # Flash signed firmware to Bank A
        add_custom_target(flash_signed_a
            COMMAND st-flash write ${PROJECT_NAME}_signed.bin 0x8008000
            DEPENDS sign_firmware
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Flashing signed firmware to Bank A (0x8008000)"
        )
        
        # Flash signed firmware to Bank B  
        add_custom_target(flash_signed_b
            COMMAND st-flash write ${PROJECT_NAME}_signed.bin 0x807C000
            DEPENDS sign_firmware
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Flashing signed firmware to Bank B (0x807C000)"
        )
        
    else()
        message(STATUS "Firmware signing disabled - no signing key provided")
        message(STATUS "Set SIGNING_KEY_PATH to enable firmware signing")
        message(STATUS "Generate keys with: make generate_keys")
    endif()
    
    # Development workflow targets
    add_custom_target(dev_setup
        DEPENDS generate_keys
        COMMENT "Setting up development environment for secure boot"
    )
    
    # Show firmware information
    add_custom_target(firmware_info
        COMMAND ${Python3_EXECUTABLE} ${FIRMWARE_VERIFY_SCRIPT} info
                --firmware ${PROJECT_NAME}_signed.bin
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Showing signed firmware information"
    )
    
else()
    message(STATUS "Python3 not found or signing script missing - secure boot targets disabled")
    message(STATUS "Install Python3 and cryptography package to enable secure boot features")
endif()

# Flash bootloader to beginning of flash
add_custom_target(flash_bootloader
    COMMAND st-flash write ${PROJECT_NAME}_bootloader.bin 0x8000000
    DEPENDS ${PROJECT_NAME}_bootloader
    COMMENT "Flashing secure bootloader to STM32F4 (0x8000000)"
)

# Flash bootloader using openocd
add_custom_target(flash_bootloader_openocd  
    COMMAND openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg
            -c "program ${PROJECT_NAME}_bootloader.hex verify reset exit"
    DEPENDS ${PROJECT_NAME}_bootloader
    COMMENT "Flashing secure bootloader using OpenOCD"
)

# Complete secure boot system flash (bootloader + signed firmware)
if(EXISTS "${SIGNING_KEY_PATH}")
    add_custom_target(flash_complete_system
        DEPENDS flash_bootloader sign_firmware
        COMMAND st-flash write ${PROJECT_NAME}_signed.bin 0x8008000
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Flashing complete secure boot system (bootloader + signed firmware)"
    )
endif()

# Help target to show available build options
add_custom_target(help_secure_boot
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Secure Boot Build Targets ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Setup:"
    COMMAND ${CMAKE_COMMAND} -E echo "  generate_keys     - Generate Ed25519 signing keypair"
    COMMAND ${CMAKE_COMMAND} -E echo "  dev_setup        - Setup development environment"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${PROJECT_NAME}            - Build main firmware"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${PROJECT_NAME}_bootloader - Build secure bootloader"
    COMMAND ${CMAKE_COMMAND} -E echo "  sign_firmware    - Sign firmware (requires SIGNING_KEY_PATH)"
    COMMAND ${CMAKE_COMMAND} -E echo "  verify_firmware  - Verify signed firmware"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Flash:"
    COMMAND ${CMAKE_COMMAND} -E echo "  flash_bootloader - Flash bootloader to 0x8000000"
    COMMAND ${CMAKE_COMMAND} -E echo "  flash_signed_a   - Flash signed firmware to Bank A"
    COMMAND ${CMAKE_COMMAND} -E echo "  flash_signed_b   - Flash signed firmware to Bank B"
    COMMAND ${CMAKE_COMMAND} -E echo "  flash_complete_system - Flash bootloader + firmware"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Debug:"
    COMMAND ${CMAKE_COMMAND} -E echo "  firmware_info    - Show signed firmware information"
    COMMAND ${CMAKE_COMMAND} -E echo "  info            - Show build information"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Configuration:"
    COMMAND ${CMAKE_COMMAND} -E echo "  FIRMWARE_VERSION=${FIRMWARE_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "  ROLLBACK_COUNTER=${ROLLBACK_COUNTER}"
    COMMAND ${CMAKE_COMMAND} -E echo "  SIGNING_KEY_PATH=${SIGNING_KEY_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "  PUBLIC_KEY_PATH=${PUBLIC_KEY_PATH}"
    COMMENT "Showing available secure boot build targets"
)